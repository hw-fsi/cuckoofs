/* Copyright (c) 2025 Huawei Technologies Co., Ltd.
 * SPDX-License-Identifier: MulanPSL-2.0
 */

#pragma once

#include <cstdint>
#include <string>

#include <sys/stat.h>
#include <sys/statvfs.h>

class Storage {
  public:
    virtual ~Storage() = default;
    virtual void DeleteInstance() = 0;
    virtual int Init() = 0;
    virtual ssize_t
    ReadObject(const std::string &objectKey, uint64_t offset, uint64_t size, int fd, char *destBuffer) = 0;
    virtual int PutFile(const std::string &objectKey, const std::string &filePath) = 0;
    virtual ssize_t
    PutBuffer(const std::string &objectKey, const char *buf, const uint64_t size, const uint64_t offset) = 0;
    virtual int DeleteObject(const std::string &objectKey) = 0;
    virtual int CopyObject(const std::string &fromPath, const std::string &toPath) = 0;
    virtual int StatFs(struct statvfs *vfsbuf) = 0;
};
