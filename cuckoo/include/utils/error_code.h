/* Copyright (c) 2025 Huawei Technologies Co., Ltd.
 * SPDX-License-Identifier: MulanPSL-2.0
 */








#ifndef CUCKOO_ERROR_CODE_H
#define CUCKOO_ERROR_CODE_H

#include "remote_connection_utils/error_code_def.h"

extern const char *CuckooErrorCodeToString[LAST_CUCKOO_ERROR_CODE + 1];

CuckooErrorCode CuckooErrorMsgAnalyse(const char* originalErrorMsg, const char** errorMsg);

#endif