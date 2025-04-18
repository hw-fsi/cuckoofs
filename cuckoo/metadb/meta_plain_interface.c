/* Copyright (c) 2025 Huawei Technologies Co., Ltd.
 * SPDX-License-Identifier: MulanPSL-2.0
 */

#include "postgres.h"

#include "fmgr.h"

#include "metadb/meta_handle.h"
#include "metadb/meta_process_info.h"
#include "utils/builtins.h"
#include "utils/error_log.h"
#include "utils/timestamp.h"
#include "utils/utils.h"

PG_FUNCTION_INFO_V1(cuckoo_plain_mkdir);
PG_FUNCTION_INFO_V1(cuckoo_plain_create);
PG_FUNCTION_INFO_V1(cuckoo_plain_stat);
PG_FUNCTION_INFO_V1(cuckoo_plain_rmdir);
PG_FUNCTION_INFO_V1(cuckoo_plain_readdir);

Datum cuckoo_plain_mkdir(PG_FUNCTION_ARGS)
{
    char *path = PG_GETARG_CSTRING(0);

    MetaProcessInfoData infoData;
    MetaProcessInfo info = &infoData;
    info->path = path;

    CuckooMkdirHandle(&info, 1);

    PG_RETURN_INT32(info->errorCode);
}

Datum cuckoo_plain_create(PG_FUNCTION_ARGS)
{
    char *path = PG_GETARG_CSTRING(0);

    MetaProcessInfoData infoData;
    MetaProcessInfo info = &infoData;
    info->path = path;

    CuckooCreateHandle(&info, 1, false);

    PG_RETURN_INT32(info->errorCode);
}

Datum cuckoo_plain_stat(PG_FUNCTION_ARGS)
{
    char *path = PG_GETARG_CSTRING(0);

    MetaProcessInfoData infoData;
    MetaProcessInfo info = &infoData;
    info->path = path;

    CuckooStatHandle(&info, 1);

    PG_RETURN_INT32(info->errorCode);
}

Datum cuckoo_plain_rmdir(PG_FUNCTION_ARGS)
{
    char *path = PG_GETARG_CSTRING(0);

    MetaProcessInfoData infoData;
    MetaProcessInfo info = &infoData;
    info->path = path;

    CuckooRmdirHandle(info);

    PG_RETURN_INT32(info->errorCode);
}

Datum cuckoo_plain_readdir(PG_FUNCTION_ARGS)
{
    char *path = PG_GETARG_CSTRING(0);

    MetaProcessInfoData infoData;
    MetaProcessInfo info = &infoData;
    info->path = path;
    info->readDirMaxReadCount = -1;
    info->readDirLastShardIndex = -1;
    info->readDirLastFileName = "";

    CuckooReadDirHandle(info);

    StringInfo result = makeStringInfo();
    for (int i = 0; i < info->readDirResultCount; ++i)
    {
        OneReadDirResult* oneReadDirResult = info->readDirResultList[i];
        appendStringInfo(result, "%s ", oneReadDirResult->fileName);
    }
    PG_RETURN_TEXT_P(cstring_to_text(result->data));
}
