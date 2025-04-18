/* Copyright (c) 2025 Huawei Technologies Co., Ltd.
 * SPDX-License-Identifier: MulanPSL-2.0
 */

#include "utils/error_log.h"

#undef CUCKOO_ERROR_CODE
#define CUCKOO_ERROR_CODE(code) #code,
const char *CuckooErrorCodeToString[LAST_CUCKOO_ERROR_CODE + 1] = {CUCKOO_ERROR_CODE_LIST};

CuckooErrorCode CuckooErrorMsgAnalyse(const char *originalErrorMsg, const char **errorMsg)
{
    if (originalErrorMsg == NULL) {
        if (errorMsg)
            *errorMsg = NULL;
        return PROGRAM_ERROR;
    }
    char errorCodeChar = originalErrorMsg[8];
    if (errorCodeChar > 64 && errorCodeChar < (64 + LAST_CUCKOO_ERROR_CODE)) {
        *errorMsg = originalErrorMsg + 9;
        return (CuckooErrorCode)(errorCodeChar - 64);
    } else {
        *errorMsg = originalErrorMsg;
        return UNKNOWN;
    }
}