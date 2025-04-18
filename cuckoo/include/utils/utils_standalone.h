/* Copyright (c) 2025 Huawei Technologies Co., Ltd.
 * SPDX-License-Identifier: MulanPSL-2.0
 */

#ifndef CUCKOO_UTILS_UTILS_STANDALONE_H
#define CUCKOO_UTILS_UTILS_STANDALONE_H

#include <stdint.h>

int64_t GetCurrentTimeInUs(void);

int64_t StringToInt64(const char* data);
uint64_t StringToUint64(const char* data);
int32_t StringToInt32(const char* data);
uint32_t StringToUint32(const char* data);

int pathcmp(const char* p1, const char* p2);

#endif