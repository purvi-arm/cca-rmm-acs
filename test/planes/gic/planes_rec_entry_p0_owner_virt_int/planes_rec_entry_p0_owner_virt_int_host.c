/*
 *
 * Copyright (c) 2024, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */
#include "test_database.h"
#include "val_host_rmi.h"
#include "val_host_helpers.h"
#include "val_irq.h"
#include "val_host_command.h"

#define TEST_IPA 0x1000

void planes_rec_entry_p0_owner_virt_int_host(void)
{
    static val_host_realm_ts realm;
    val_host_realm_flags1_ts realm_flags;
    uint64_t ret;
    val_smc_param_ts cmd_ret;
    val_host_rec_exit_ts *rec_exit = NULL;
    val_host_rec_enter_ts *rec_enter = NULL;
    uint64_t s2ap_ipa_base, s2ap_ipa_top;
    uint64_t phys;

    /* Skip if RMM do not support planes */
    if (!val_host_rmm_supports_planes())
    {
        LOG(ALWAYS, "\n\tPlanes feature not supported\n", 0, 0);
        val_set_status(RESULT_SKIP(VAL_SKIP_CHECK));
        goto destroy_realm;
    }

    val_memset(&realm, 0, sizeof(realm));
    val_memset(&realm_flags, 0, sizeof(realm_flags));

    val_host_realm_params(&realm);

    /* Overwrite Realm Parameters */
    realm.num_aux_planes = 1;
    realm_flags.rtt_tree_pp = RMI_FEATURE_TRUE;

    if (val_host_rmm_supports_rtt_tree_single())
        realm_flags.rtt_tree_pp = RMI_FEATURE_FALSE;

    val_memcpy(&realm.flags1, &realm_flags, sizeof(realm.flags1));

    LOG(DBG, "\t INFO: RTT tree per plane : %d\n", realm_flags.rtt_tree_pp, 0);
    /* Populate realm with one REC*/
    if (val_host_realm_setup(&realm, false))
    {
        LOG(ERROR, "\tRealm setup failed\n", 0, 0);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(1)));
        goto destroy_realm;
    }

    /* Prepare IPA whose HIPAS = UNASIGNED, RIPAS = RAM, the process to put the IPA
     * to this state depends on whether we are acccessing primary or auxiliary RTT tree */
    if (val_host_ripas_init(&realm, TEST_IPA, TEST_IPA + PAGE_SIZE,
                                                     VAL_RTT_MAX_LEVEL, PAGE_SIZE))
    {
            LOG(ERROR, "\tRMI_INIT_RIPAS failed ", 0, 0);
            val_set_status(RESULT_FAIL(VAL_ERROR_POINT(2)));
            goto destroy_realm;
    }

    /* Activate realm */
    if (val_host_realm_activate(&realm))
    {
        LOG(ERROR, "\tRealm activate failed\n", 0, 0);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(3)));
        goto destroy_realm;
    }

    rec_enter = &(((val_host_rec_run_ts *)realm.run[0])->enter);
    rec_exit = &(((val_host_rec_run_ts *)realm.run[0])->exit);

    /* Enter REC[0]  */
    ret = val_host_rmi_rec_enter(realm.rec[0], realm.run[0]);
    if (ret)
    {
        LOG(ERROR, "\tRec enter failed, ret=%x\n", ret, 0);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(4)));
        goto destroy_realm;
    }

    /* Check that REC Exit was due to host call because of P0 requesting for test IPA */
    if (rec_exit->exit_reason != RMI_EXIT_HOST_CALL) {
        LOG(ERROR, "\tUnexpected REC exit, %d. ESR: %lx \n", rec_exit->exit_reason, rec_exit->esr);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(5)));
        goto destroy_realm;
    }

    /* Return the test IPA to P0 */
    rec_enter->gprs[1] = TEST_IPA;
    rec_enter->gprs[2] = PAGE_SIZE;

    /* Enter REC[0]  */
    ret = val_host_rmi_rec_enter(realm.rec[0], realm.run[0]);
    if (ret)
    {
        LOG(ERROR, "\tRec enter failed, ret=%x\n", ret, 0);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(6)));
        goto destroy_realm;
    }

    /* Check that REC exit was due S2AP change request */
    if (rec_exit->exit_reason != RMI_EXIT_S2AP_CHANGE) {
        LOG(ERROR, "\tUnexpected REC exit, %d. ESR: %lx \n", rec_exit->exit_reason, rec_exit->esr);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(7)));
        goto destroy_realm;
    }

    s2ap_ipa_base = rec_exit->s2ap_base;
    s2ap_ipa_top =  rec_exit->s2ap_top;

    while (s2ap_ipa_base != s2ap_ipa_top) {
        cmd_ret = val_host_rmi_rtt_set_s2ap(realm.rd, realm.rec[0], s2ap_ipa_base, s2ap_ipa_top);
        if (cmd_ret.x0) {
            LOG(ERROR, "\nRMI_SET_S2AP failed with ret= 0x%x\n", cmd_ret.x0, 0);
            val_set_status(RESULT_FAIL(VAL_ERROR_POINT(8)));
            goto destroy_realm;
        }
        s2ap_ipa_base = cmd_ret.x1;
    }

    rec_enter->flags = 0x0;

    /* Enter REC[0]  */
    ret = val_host_rmi_rec_enter(realm.rec[0], realm.run[0]);
    if (ret)
    {
        LOG(ERROR, "\tRec enter failed, ret=%x\n", ret, 0);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(9)));
        goto destroy_realm;
    }

    /* Check that REC exit was due S2AP change request */
    if (rec_exit->exit_reason != RMI_EXIT_S2AP_CHANGE) {
        LOG(ERROR, "\tUnexpected REC exit, %d. ESR: %lx \n", rec_exit->exit_reason, rec_exit->esr);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(10)));
        goto destroy_realm;
    }

    s2ap_ipa_base = rec_exit->s2ap_base;
    s2ap_ipa_top =  rec_exit->s2ap_top;

    while (s2ap_ipa_base != s2ap_ipa_top) {
        cmd_ret = val_host_rmi_rtt_set_s2ap(realm.rd, realm.rec[0], s2ap_ipa_base, s2ap_ipa_top);

        /* RTT_SET_S2AP requires all RTTs to be create when running in RTT per plane
         * configuration */
        if (RMI_STATUS(cmd_ret.x0) == RMI_ERROR_RTT)
        {
                if (create_mapping(s2ap_ipa_base, false, realm.rd))
                {
                    LOG(ERROR, "\tRTT_AUX_CREATE failed\n", 0, 0);
                    val_set_status(RESULT_FAIL(VAL_ERROR_POINT(11)));
                    goto destroy_realm;
                }
            continue;
        }
        else if (RMI_STATUS(cmd_ret.x0) == RMI_ERROR_RTT_AUX)
        {
            for (uint64_t i = 0; i < realm.num_aux_planes; i++)
            {
                if (val_host_create_aux_mapping(realm.rd, s2ap_ipa_base, i + 1))
                {
                    LOG(ERROR, "\tRTT_AUX_CREATE failed\n", 0, 0);
                    val_set_status(RESULT_FAIL(VAL_ERROR_POINT(12)));
                    goto destroy_realm;
                }
            }

            continue;
        }
        else if (RMI_STATUS(cmd_ret.x0) == RMI_ERROR_INPUT) {
            LOG(ERROR, "\nRMI_SET_S2AP failed with ret= 0x%x\n", cmd_ret.x0, 0);
            val_set_status(RESULT_FAIL(VAL_ERROR_POINT(13)));
            goto destroy_realm;
        }

        s2ap_ipa_base = cmd_ret.x1;
    }

    rec_enter->flags = 0x0;

    /* Enter REC[0]  */
    ret = val_host_rmi_rec_enter(realm.rec[0], realm.run[0]);
    if (ret)
    {
        LOG(ERROR, "\tRec enter failed, ret=%x\n", ret, 0);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(14)));
        goto destroy_realm;
    }

    /* Check that REC exit was due to Data Abort due to P1 access to IPA whose
     * HIPAS,RIPAS = UNASSIGNED,RAM */
    if (validate_rec_exit_da(rec_exit, TEST_IPA, ESR_ISS_DFSC_TTF_L3,
                                NON_EMULATABLE_DA, ESR_WnR_WRITE))
    {
        LOG(ERROR, "\tREC exit DA: params mismatch\n", 0, 0);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(15)));
        goto destroy_realm;
    }

    /* Fix the Abort */
    phys = (uint64_t)val_host_mem_alloc(PAGE_SIZE, PAGE_SIZE);
    if (!phys)
    {
        LOG(ERROR, "\tval_host_mem_alloc failed\n", 0, 0);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(16)));
        goto destroy_realm;
    }

    ret = val_host_map_protected_data_unknown(&realm, phys, TEST_IPA, PAGE_SIZE);
    if (ret)
    {
        LOG(ERROR, "\tDATA_CREATE_UNKNOWN failed, ret = %d \n", ret, 0);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(17)));
        goto destroy_realm;
    }

    /* Test Intent : REC entry with one virtual interrupt pending,
     *  i.e entry.gicv3_lrs[x].State = PENDING */
    rec_enter->gicv3_lrs[0] = ((uint64_t)GICV3_LR_STATE_PENDING << GICV3_LR_STATE)
                                  | (0ULL << GICV3_LR_HW) | (1ULL << GICV3_LR_GROUP) | PPI_vINTID;

    /* Enter REC[0]  */
    ret = val_host_rmi_rec_enter(realm.rec[0], realm.run[0]);
    if (ret)
    {
        LOG(ERROR, "\tRec enter failed, ret=%x\n", ret, 0);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(18)));
        goto destroy_realm;
    }

    /* Check that REC exit was due to Host call form P0 after completing test */
    if (rec_exit->exit_reason != RMI_EXIT_HOST_CALL) {
        LOG(ERROR, "\tUnexpected REC exit, %d. ESR: %lx \n", rec_exit->exit_reason, rec_exit->esr);
        val_set_status(RESULT_FAIL(VAL_ERROR_POINT(19)));
        goto destroy_realm;
    }

    val_set_status(RESULT_PASS(VAL_SUCCESS));

    /* Free test resources */
destroy_realm:
    return;
}