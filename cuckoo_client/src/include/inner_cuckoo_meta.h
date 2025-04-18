/* Copyright (c) 2025 Huawei Technologies Co., Ltd.
 * SPDX-License-Identifier: MulanPSL-2.0
 */

#pragma once

#include <sys/stat.h>
#include <sys/statvfs.h>
#include <sys/time.h>

#include "buffer/open_instance.h"

struct BatchCreatePrams
{
    std::vector<std::string> paths;
    std::vector<std::string> etags;
    std::vector<uint64_t> sizes;
    std::vector<int64_t> mtimes;

    bool operator==(const BatchCreatePrams &other) const { return paths == other.paths; }
};

int InnerCuckooUnlink(uint64_t inodeId, int nodeId, std::string path);
int InnerCuckooWrite(OpenInstance *openInstance, const char *buffer, size_t size, off_t offset);
int InnerCuckooTmpClose(OpenInstance *openInstance, bool isFlush, bool isSync);
int InnerCuckooRead(OpenInstance *openInstance, char *buffer, size_t size, off_t offset);
int InnerCuckooAsyncCopy(uint64_t inodeId, int &backupNodeId);

int InnerCuckooReadSmallFiles(OpenInstance *openInstance);
int InnerCuckooStatFS(struct statvfs *vfsbuf);
int InnerCuckooCopydata(const std::string &srcName, const std::string &dstName);
int InnerCuckooDeleteDataAfterRename(const std::string &objectName);
int InnerCuckooTruncateOpenInstance(OpenInstance *openInstance, off_t size);
int InnerCuckooTruncateFile(OpenInstance *openInstance, off_t size);
