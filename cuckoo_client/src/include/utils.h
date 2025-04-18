/* Copyright (c) 2025 Huawei Technologies Co., Ltd.
 * SPDX-License-Identifier: MulanPSL-2.0
 */

#pragma once

#include <stdint.h>

#include <sys/time.h>

#define likely(x) __builtin_expect((x) != 0, 1)
#define unlikely(x) __builtin_expect((x) != 0, 0)

int64_t StringToInt64(const char *data);
uint64_t StringToUint64(const char *data);
int32_t StringToInt32(const char *data);
uint32_t StringToUint32(const char *data);
bool StringToBool(const char *data);

#define FILENAME_LENGTH 256

uint16_t HashPartId(const char *fileName);
uint32_t HashInt8(int64_t val);
int StrnLen(const char *str, int maxLen);
