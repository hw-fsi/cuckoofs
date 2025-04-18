/* Copyright (c) 2025 Huawei Technologies Co., Ltd.
 * SPDX-License-Identifier: MulanPSL-2.0
 */

#include "control/control_flag.h"

#include "fmgr.h"
#include "port/atomics.h"
#include "storage/lwlock.h"
#include "storage/shmem.h"

#include "utils/shmem_control.h"

static ShmemControlData *CuckooControlShmemControl;
static pg_atomic_uint32 *CuckooBackgroundServiceStarted;

PG_FUNCTION_INFO_V1(cuckoo_start_background_service);
Datum cuckoo_start_background_service(PG_FUNCTION_ARGS)
{
    pg_atomic_exchange_u32(CuckooBackgroundServiceStarted, 1);
    PG_RETURN_INT16(0);
}

bool CheckCuckooBackgroundServiceStarted() { return pg_atomic_read_u32(CuckooBackgroundServiceStarted) != 0; }

size_t CuckooControlShmemsize() { return sizeof(ShmemControlData) + sizeof(pg_atomic_uint32); }
void CuckooControlShmemInit()
{
    bool initialized = false;

    CuckooControlShmemControl = ShmemInitStruct("Cuckoo Control", CuckooControlShmemsize(), &initialized);
    CuckooBackgroundServiceStarted = (pg_atomic_uint32 *)(CuckooControlShmemControl + 1);
    if (!initialized) {
        CuckooControlShmemControl->trancheId = LWLockNewTrancheId();
        CuckooControlShmemControl->lockTrancheName = "Cuckoo Control";
        LWLockRegisterTranche(CuckooControlShmemControl->trancheId, CuckooControlShmemControl->lockTrancheName);
        LWLockInitialize(&CuckooControlShmemControl->lock, CuckooControlShmemControl->trancheId);

        pg_atomic_init_u32(CuckooBackgroundServiceStarted, 0);
    }
}

static bool CuckooInAbortProgress = false;
bool CuckooIsInAbortProgress(void) { return CuckooInAbortProgress; }
void CuckooEnterAbortProgress(void) { CuckooInAbortProgress = true; }
void CuckooQuitAbortProgress(void) { CuckooInAbortProgress = false; }