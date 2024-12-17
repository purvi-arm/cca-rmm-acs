/*
 * Copyright (c) 2023-2024, Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#include "val_rmm.h"

enum test_intent {
    PARAMS_UNALIGNED = 0X0,
    PARAMS_DEV_MEM = 0X1,
    PARAMS_OUTSIDE_OF_PERMITTED_PA = 0X2,
    PARAMS_PAS_REALM = 0X3,
    PARAMS_PAS_SECURE = 0X4,
    REC_UNALIGNED = 0X5,
    REC_DEV_MEM = 0X6,
    REC_OUTSIDE_OF_PERMITTED_PA = 0X7,
    REC_GRAN_STATE_UNDELEGATED = 0X8,
    REC_GRAN_STATE_RD = 0X9,
    REC_GRAN_STATE_REC = 0XA,
    REC_GRAN_STATE_RTT = 0XB,
    REC_GRAN_STATE_DATA = 0XC,
    RD_UNALIGNED = 0XD,
    RD_DEV_MEM = 0XE,
    RD_OUTSIDE_OF_PERMITTED_PA = 0XF,
    RD_STATE_UNDELEGATED = 0X10,
    RD_STATE_DELEGATED = 0X11,
    RD_STATE_REC = 0X12,
    RD_STATE_RTT = 0X13,
    RD_STATE_DATA = 0X14,
    REALM_ACTIVE = 0X15,
    REALM_SYSTEM_OFF = 0X16,
    MPIDR_SKIPPED = 0X17,
    NUM_AUX_INVALID = 0X18,
    AUX_UNALIGNED = 0X19,
    AUX_ALIASED = 0X1A,
    AUX_UNDELEGATED = 0X1B,
    AUX_RD = 0X1C,
    AUX_REC = 0X1D,
    AUX_RTT = 0X1E,
    AUX_DATA = 0X1F
};

struct stimulus {
    char msg[100];
    uint64_t abi;
    enum test_intent label;
    uint64_t status;
    uint64_t index;
};

static struct stimulus test_data[] = {
    {.msg = "params_align",
    .abi = RMI_REC_CREATE,
    .label = PARAMS_UNALIGNED,
    .status = RMI_ERROR_INPUT,
    .index = 0},
    {.msg = "params_bound",
    .abi = RMI_REC_CREATE,
    .label = PARAMS_DEV_MEM,
    .status = RMI_ERROR_INPUT,
    .index = 0},
    {.msg = "params_bound",
    .abi = RMI_REC_CREATE,
    .label = PARAMS_OUTSIDE_OF_PERMITTED_PA,
    .status = RMI_ERROR_INPUT,
    .index = 0},
    {.msg = "params_pas",
    .abi = RMI_REC_CREATE,
    .label = PARAMS_PAS_REALM,
    .status = RMI_ERROR_INPUT,
    .index = 0},
    {.msg = "params_pas",
    .abi = RMI_REC_CREATE,
    .label = PARAMS_PAS_SECURE,
    .status = RMI_ERROR_INPUT,
    .index = 0},
    {.msg = "rec_align",
    .abi = RMI_REC_CREATE,
    .label = REC_UNALIGNED,
    .status = RMI_ERROR_INPUT,
    .index = 0},
    {.msg = "rec_bound",
    .abi = RMI_REC_CREATE,
    .label = REC_DEV_MEM,
    .status = RMI_ERROR_INPUT,
    .index = 0},
    {.msg = "rec_bound",
    .abi = RMI_REC_CREATE,
    .label = REC_OUTSIDE_OF_PERMITTED_PA,
    .status = RMI_ERROR_INPUT,
    .index = 0},
    {.msg = "rec_state",
    .abi = RMI_REC_CREATE,
    .label = REC_GRAN_STATE_UNDELEGATED,
    .status = RMI_ERROR_INPUT,
    .index = 0},
    {.msg = "rec_state",
    .abi = RMI_REC_CREATE,
    .label = REC_GRAN_STATE_RD,
    .status = RMI_ERROR_INPUT,
    .index = 0},
    {.msg = "rec_state",
    .abi = RMI_REC_CREATE,
    .label = REC_GRAN_STATE_REC,
    .status = RMI_ERROR_INPUT,
    .index = 0},
    {.msg = "rec_state",
    .abi = RMI_REC_CREATE,
    .label = REC_GRAN_STATE_RTT,
    .status = RMI_ERROR_INPUT,
    .index = 0},
    {.msg = "rec_state",
    .abi = RMI_REC_CREATE,
    .label = REC_GRAN_STATE_DATA,
    .status = RMI_ERROR_INPUT,
    .index = 0},
    {.msg = "rd_align",
    .abi = RMI_REC_CREATE,
    .label = RD_UNALIGNED,
    .status = RMI_ERROR_INPUT,
    .index = 0},
    {.msg = "rd_bound",
    .abi = RMI_REC_CREATE,
    .label = RD_DEV_MEM,
    .status = RMI_ERROR_INPUT,
    .index = 0},
    {.msg = "rd_bound",
    .abi = RMI_REC_CREATE,
    .label = RD_OUTSIDE_OF_PERMITTED_PA,
    .status = RMI_ERROR_INPUT,
    .index = 0},
    {.msg = "rd_state",
    .abi = RMI_REC_CREATE,
    .label = RD_STATE_UNDELEGATED,
    .status = RMI_ERROR_INPUT,
    .index = 0},
    {.msg = "rd_state",
    .abi = RMI_REC_CREATE,
    .label = RD_STATE_DELEGATED,
    .status = RMI_ERROR_INPUT,
    .index = 0},
    {.msg = "rd_state",
    .abi = RMI_REC_CREATE,
    .label = RD_STATE_REC,
    .status = RMI_ERROR_INPUT,
    .index = 0},
    {.msg = "rd_state",
    .abi = RMI_REC_CREATE,
    .label = RD_STATE_RTT,
    .status = RMI_ERROR_INPUT,
    .index = 0},
    {.msg = "rd_state",
    .abi = RMI_REC_CREATE,
    .label = RD_STATE_DATA,
    .status = RMI_ERROR_INPUT,
    .index = 0},
    {.msg = "realm_state",
    .abi = RMI_REC_CREATE,
    .label = REALM_ACTIVE,
    .status = RMI_ERROR_REALM,
    .index = 0},
    {.msg = "realm_state",
    .abi = RMI_REC_CREATE,
    .label = REALM_SYSTEM_OFF,
    .status = RMI_ERROR_REALM,
    .index = 0},
    {.msg = "mpidr_index",
    .abi = RMI_REC_CREATE,
    .label = MPIDR_SKIPPED,
    .status = RMI_ERROR_INPUT,
    .index = 0},
    {.msg = "num_aux",
    .abi = RMI_REC_CREATE,
    .label = NUM_AUX_INVALID,
    .status = RMI_ERROR_INPUT,
    .index = 0},
    {.msg = "aux_align",
    .abi = RMI_REC_CREATE,
    .label = AUX_UNALIGNED,
    .status = RMI_ERROR_INPUT,
    .index = 0},
//    {.msg = "aux_alias",
//    .abi = RMI_REC_CREATE,
//    .label = AUX_ALIASED,
//    .status = RMI_ERROR_INPUT,
//    .index = 0},
    {.msg = "aux_state",
    .abi = RMI_REC_CREATE,
    .label = AUX_UNDELEGATED,
    .status = RMI_ERROR_INPUT,
    .index = 0},
    {.msg = "aux_state",
    .abi = RMI_REC_CREATE,
    .label = AUX_RD,
    .status = RMI_ERROR_INPUT,
    .index = 0},
    {.msg = "aux_state",
    .abi = RMI_REC_CREATE,
    .label = AUX_REC,
    .status = RMI_ERROR_INPUT,
    .index = 0},
    {.msg = "aux_state",
    .abi = RMI_REC_CREATE,
    .label = AUX_RTT,
    .status = RMI_ERROR_INPUT,
    .index = 0},
    {.msg = "aux_state",
    .abi = RMI_REC_CREATE,
    .label = AUX_DATA,
    .status = RMI_ERROR_INPUT,
    .index = 0}
};
