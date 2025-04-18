/* Copyright (c) 2025 Huawei Technologies Co., Ltd.
 * SPDX-License-Identifier: MulanPSL-2.0
 */

#include "utils/utils_standalone.h"

#include <stdatomic.h>
#include <stdint.h>
#include <stdlib.h>
#include <sys/time.h>

int64_t GetCurrentTimeInUs()
{
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return tv.tv_sec * 1000000 + tv.tv_usec;
}

int64_t StringToInt64(const char *data) { return strtoll(data, NULL, 10); }
uint64_t StringToUint64(const char *data) { return strtoull(data, NULL, 10); }
int32_t StringToInt32(const char *data) { return strtol(data, NULL, 10); }
uint32_t StringToUint32(const char *data) { return strtoul(data, NULL, 10); }

int pathcmp(const char *p1, const char *p2)
{
    const unsigned char *s1 = (const unsigned char *)p1;
    const unsigned char *s2 = (const unsigned char *)p2;
    unsigned char c1, c2;
    do {
        c1 = (unsigned char)*s1++;
        c2 = (unsigned char)*s2++;
        if (c1 == '\0')
            return c1 - c2;
    } while (c1 == c2);

    if (c2 == '\0' || c2 == '/')
        return 1;
    if (c1 == '/')
        return -1;
    return c1 - c2;
}