/* Copyright (c) 2025 Huawei Technologies Co., Ltd.
 * SPDX-License-Identifier: MulanPSL-2.0
 */

#ifndef CUCKOO_UTILS_RWLOCK_H
#define CUCKOO_UTILS_RWLOCK_H

#include "postgres.h"
#include "port/atomics.h"

typedef struct RWLock
{
    pg_atomic_uint64 state;
} RWLock;

typedef enum RWLockMode
{
    RW_EXCLUSIVE,
    RW_SHARED,

    RW_DECLARE,
} RWLockMode;

extern void RWLockInitialize(RWLock *lock);
void RWLockDeclare(RWLock *lock);
void RWLockUndeclare(RWLock *lock);
bool RWLockCheckDestroyable(RWLock *lock);
void RWLockAcquire(RWLock *lock, RWLockMode mode);
void RWLockRelease(RWLock *lock);
void RWLockReleaseAll(bool keepInterruptHoldoffCount);

#endif