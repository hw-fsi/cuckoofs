/* Copyright (c) 2025 Huawei Technologies Co., Ltd.
 * SPDX-License-Identifier: MulanPSL-2.0
 */

#ifndef CUCKOO_CONTROL_FLAG_H
#define CUCKOO_CONTROL_FLAG_H

#include "postgres.h"

size_t CuckooControlShmemsize(void);
void CuckooControlShmemInit(void);

bool CheckCuckooBackgroundServiceStarted(void);

bool CuckooIsInAbortProgress(void);
void CuckooEnterAbortProgress(void);
void CuckooQuitAbortProgress(void);

#endif