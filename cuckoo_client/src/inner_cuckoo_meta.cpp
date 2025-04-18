/* Copyright (c) 2025 Huawei Technologies Co., Ltd.
 * SPDX-License-Identifier: MulanPSL-2.0
 */

#include "inner_cuckoo_meta.h"

#include "cuckoo_store/cuckoo_store.h"

int InnerCuckooWrite(OpenInstance *openInstance, const char *buffer, size_t size, off_t offset)
{
    return CuckooStore::GetInstance()->WriteFile(openInstance, buffer, size, offset);
}

int InnerCuckooTmpClose(OpenInstance *openInstance, bool isFlush, bool isSync)
{
    return CuckooStore::GetInstance()->CloseTmpFiles(openInstance, isFlush, isSync);
}

int InnerCuckooRead(OpenInstance *openInstance, char *buffer, size_t size, off_t offset)
{
    return CuckooStore::GetInstance()->ReadFile(openInstance, buffer, size, offset);
}

int InnerCuckooReadSmallFiles(OpenInstance *openInstance)
{
    return CuckooStore::GetInstance()->ReadSmallFiles(openInstance);
}

int InnerCuckooStatFS(struct statvfs *vfsbuf) { return CuckooStore::GetInstance()->StatFS(vfsbuf); }

int InnerCuckooCopydata(const std::string &srcName, const std::string &dstName)
{
    return CuckooStore::GetInstance()->CopyData(srcName, dstName);
}

int InnerCuckooDeleteDataAfterRename(const std::string &objectName)
{
    int deleteRet = CuckooStore::GetInstance()->DeleteDataAfterRename(objectName);
    if (deleteRet != 0) {
        CUCKOO_LOG(LOG_ERROR) << "delete object: " << objectName << " in rename failed!";
    }
    return deleteRet;
}

int InnerCuckooTruncateOpenInstance(OpenInstance *openInstance, off_t size)
{
    return CuckooStore::GetInstance()->TruncateOpenInstance(openInstance, size);
}

int InnerCuckooTruncateFile(OpenInstance *openInstance, off_t size)
{
    return CuckooStore::GetInstance()->TruncateFile(openInstance, size);
}

int InnerCuckooUnlink(uint64_t inodeId, int nodeId, std::string path)
{
    return CuckooStore::GetInstance()->DeleteFiles(inodeId, nodeId, path);
}
