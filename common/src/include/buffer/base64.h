/* Copyright (c) 2025 Huawei Technologies Co., Ltd.
 * SPDX-License-Identifier: MulanPSL-2.0
 */

#pragma once

#ifndef UINT32_MAX
#define UINT32_MAX (4294967295U)
#endif

#define BASE64_ENCODE_OUT_SIZE(s) ((unsigned int)((((s) + 2) / 3) * 4 + 1))
#define BASE64_DECODE_OUT_SIZE(s) ((unsigned int)(((s) / 4) * 3))

unsigned int base64_encode(const unsigned char *in, unsigned int inlen, char *out);

unsigned int base64_decode(const char *in, unsigned int inlen, unsigned char *out);
