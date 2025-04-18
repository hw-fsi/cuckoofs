/* Copyright (c) 2025 Huawei Technologies Co., Ltd.
 * SPDX-License-Identifier: MulanPSL-2.0
 */

#include "utils.h"

#include <stdlib.h>
#include <cstring>

#include <sys/time.h>

static inline uint32_t RotateLeft32(uint32_t word, int n);
static uint32_t HashBytesUint32(uint32_t k);

int64_t StringToInt64(const char *data) { return strtoll(data, nullptr, 10); }
uint64_t StringToUint64(const char *data) { return strtoull(data, nullptr, 10); }
int StringToInt32(const char *data) { return strtol(data, nullptr, 10); }
uint32_t StringToUint32(const char *data) { return strtoul(data, nullptr, 10); }
bool StringToBool(const char *data) { return data[0] == 't' || data[0] == 'T'; }

uint16_t HashPartId(const char *fileName)
{
    uint16_t hashValue = 0;
    for (int i = 0; i < StrnLen(fileName, FILENAME_LENGTH); ++i) {
        hashValue = hashValue * 31 + fileName[i];
    }
    return hashValue & 0x1FFF;
}

uint32_t HashInt8(int64_t val)
{
    auto lohalf = static_cast<uint32_t>(val);
    auto hihalf = static_cast<uint32_t>(val >> 32);

    lohalf ^= (val >= 0) ? hihalf : ~hihalf;

    int32_t res = HashBytesUint32(lohalf);

    res &= ~(1u << 31);
    return res;
}

int StrnLen(const char *str, int maxLen)
{
    const char *p = str;

    while (maxLen-- > 0 && *p) {
        p++;
    }
    return p - str;
}

static uint32_t HashBytesUint32(uint32_t k)
{
    uint32_t a;
    uint32_t b;
    uint32_t c;

    a = b = c = 0x9e3779b9 + static_cast<uint32_t>(sizeof(uint32_t)) + 3923095;
    a += k;

    c ^= b;
    c -= RotateLeft32(b, 14);
    a ^= c;
    a -= RotateLeft32(c, 11);
    b ^= a;
    b -= RotateLeft32(a, 25);
    c ^= b;
    c -= RotateLeft32(b, 16);
    a ^= c;
    a -= RotateLeft32(c, 4);
    b ^= a;
    b -= RotateLeft32(a, 14);
    c ^= b;
    c -= RotateLeft32(b, 24);

    return c;
}

static inline uint32_t RotateLeft32(uint32_t word, int n) { return (word << n) | (word >> (32 - n)); }
