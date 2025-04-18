/* Copyright (c) 2025 Huawei Technologies Co., Ltd.
 * SPDX-License-Identifier: MulanPSL-2.0
 */

#include "postgres.h"

#include "fmgr.h"
#include "utils/palloc.h"

#include <unistd.h>

#include "connection_pool/connection_pool.h"
#include "metadb/meta_serialize_interface_helper.h"
#include "utils/error_log.h"

PG_FUNCTION_INFO_V1(cuckoo_meta_call_by_serialized_shmem_internal);
PG_FUNCTION_INFO_V1(cuckoo_meta_call_by_serialized_data);

static SerializedData MetaProcess(CuckooSupportMetaService metaService, int count, char *paramBuffer)
{
    if (count != 1 && !(metaService == MKDIR || metaService == MKDIR_SUB_MKDIR || metaService == MKDIR_SUB_CREATE ||
                        metaService == CREATE || metaService == STAT || metaService == OPEN || metaService == CLOSE ||
                        metaService == UNLINK))
        CUCKOO_ELOG_ERROR_EXTENDED(ARGUMENT_ERROR, "metaService %d doesn't support batch operation.", metaService);

    SerializedData param;

    if (!SerializedDataInit(&param, paramBuffer, SD_SIZE_T_MAX, SD_SIZE_T_MAX, NULL))
        CUCKOO_ELOG_ERROR(ARGUMENT_ERROR, "SerializedDataInit failed.");

    void *data = palloc((sizeof(MetaProcessInfoData) + sizeof(MetaProcessInfoData *)) * count);
    MetaProcessInfoData *infoDataArray = data;
    MetaProcessInfo *infoArray = (MetaProcessInfo *)(infoDataArray + count);
    if (!SerializedDataMetaParamDecode(metaService, count, &param, infoDataArray))
        CUCKOO_ELOG_ERROR(ARGUMENT_ERROR, "serialized param is corrupt.");
    for (int i = 0; i < count; i++)
        infoArray[i] = infoDataArray + i;

    switch (metaService) {
    case MKDIR:
        CuckooMkdirHandle(infoArray, count);
        break;
    case MKDIR_SUB_MKDIR:
        CuckooMkdirSubMkdirHandle(infoArray, count);
        break;
    case MKDIR_SUB_CREATE:
        CuckooMkdirSubCreateHandle(infoArray, count);
        break;
    case CREATE:
        CuckooCreateHandle(infoArray, count, false);
        break;
    case STAT:
        CuckooStatHandle(infoArray, count);
        break;
    case OPEN:
        CuckooOpenHandle(infoArray, count);
        break;
    case CLOSE:
        CuckooCloseHandle(infoArray, count);
        break;
    case UNLINK:
        CuckooUnlinkHandle(infoArray, count);
        break;
    case READDIR:
        CuckooReadDirHandle(infoArray[0]);
        break;
    case OPENDIR:
        CuckooOpenDirHandle(infoArray[0]);
        break;
    case RMDIR:
        CuckooRmdirHandle(infoArray[0]);
        break;
    case RMDIR_SUB_RMDIR:
        CuckooRmdirSubRmdirHandle(infoArray[0]);
        break;
    case RMDIR_SUB_UNLINK:
        CuckooRmdirSubUnlinkHandle(infoArray[0]);
        break;
    case RENAME:
        CuckooRenameHandle(infoArray[0]);
        break;
    case RENAME_SUB_RENAME_LOCALLY:
        CuckooRenameSubRenameLocallyHandle(infoArray[0]);
        break;
    case RENAME_SUB_CREATE:
        CuckooRenameSubCreateHandle(infoArray[0]);
        break;
    case UTIMENS:
        CuckooUtimeNsHandle(infoArray[0]);
        break;
    case CHOWN:
        CuckooChownHandle(infoArray[0]);
        break;
    case CHMOD:
        CuckooChmodHandle(infoArray[0]);
        break;
    default:
        CUCKOO_ELOG_ERROR_EXTENDED(ARGUMENT_ERROR, "unexpected metaService: %d", metaService);
    }

    SerializedData response;
    SerializedDataInit(&response, NULL, 0, 0, &PgMemoryManager);
    if (!SerializedDataMetaResponseEncodeWithPerProcessFlatBufferBuilder(metaService, count, infoDataArray, &response))
        CUCKOO_ELOG_ERROR(ARGUMENT_ERROR, "failed when serializing response.");

    return response;
}

Datum cuckoo_meta_call_by_serialized_shmem_internal(PG_FUNCTION_ARGS)
{
    int32_t type = PG_GETARG_INT32(0);
    int32_t count = PG_GETARG_INT32(1);
    uint64_t paramShmemShift = (uint64_t)PG_GETARG_INT64(2);
    int64_t signature = PG_GETARG_INT64(3);

    CuckooSupportMetaService metaService = MetaServiceTypeDecode(type);

    CuckooShmemAllocator *allocator = &CuckooConnectionPoolShmemAllocator;
    if (paramShmemShift > allocator->pageCount * CUCKOO_SHMEM_ALLOCATOR_PAGE_SIZE)
        CUCKOO_ELOG_ERROR(ARGUMENT_ERROR, "paramShmemShift is invalid.");
    char *paramBuffer = CUCKOO_SHMEM_ALLOCATOR_GET_POINTER(allocator, paramShmemShift);

    SerializedData response = MetaProcess(metaService, count, paramBuffer);

    uint64_t responseShmemShift = CuckooShmemAllocatorMalloc(allocator, response.size);
    if (responseShmemShift == 0)
        CUCKOO_ELOG_ERROR_EXTENDED(PROGRAM_ERROR, "CuckooShmemAllocMalloc failed. Size: %u.", response.size);
    char *responseBuffer = CUCKOO_SHMEM_ALLOCATOR_GET_POINTER(allocator, responseShmemShift);
    CUCKOO_SHMEM_ALLOCATOR_SET_SIGNATURE(responseBuffer, signature);
    memcpy(responseBuffer, response.buffer, response.size);

    PG_RETURN_INT64(responseShmemShift);
}

Datum cuckoo_meta_call_by_serialized_data(PG_FUNCTION_ARGS)
{
    int32_t type = PG_GETARG_INT32(0);
    int32_t count = PG_GETARG_INT32(1);
    bytea *param = PG_GETARG_BYTEA_P(2);

    CuckooSupportMetaService metaService = MetaServiceTypeDecode(type);
    char *paramBuffer = VARDATA_ANY(param);

    SerializedData response = MetaProcess(metaService, count, paramBuffer);

    bytea *reply = (bytea *)palloc(VARHDRSZ + response.size);
    memcpy(VARDATA_4B(reply), response.buffer, response.size);
    SET_VARSIZE_4B(reply, VARHDRSZ + response.size);

    PG_RETURN_BYTEA_P(reply);
}