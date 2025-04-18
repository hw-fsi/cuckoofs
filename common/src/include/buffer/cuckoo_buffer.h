/* Copyright (c) 2025 Huawei Technologies Co., Ltd.
 * SPDX-License-Identifier: MulanPSL-2.0
 */

#pragma once

#include <cstddef>

struct CuckooWriteBuffer
{
    const char *ptr = nullptr;
    size_t size = 0;
};

struct CuckooReadBuffer
{
    char *ptr = nullptr;
    size_t size = 0;
};
