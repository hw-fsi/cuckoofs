/* Copyright (c) 2025 Huawei Technologies Co., Ltd.
 * SPDX-License-Identifier: MulanPSL-2.0
 */

#ifndef CUCKOO_SHMEM_CONTROL_H
#define CUCKOO_SHMEM_CONTROL_H

#include "storage/lwlock.h"

typedef struct ShmemControlData 
{
    int trancheId;
    char* lockTrancheName;
    LWLock lock;
} ShmemControlData;

#endif