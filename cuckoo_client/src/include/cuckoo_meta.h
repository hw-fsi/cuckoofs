/* Copyright (c) 2025 Huawei Technologies Co., Ltd.
 * SPDX-License-Identifier: MulanPSL-2.0
 */

#pragma once

#include <stdint.h>
#include <memory>

#include "router.h"

extern std::shared_ptr<Router> router;

struct CuckooFuseInfo
{
    int flags;

    unsigned long fhOld;

    int writePage;

    unsigned int directIO : 1;

    unsigned int keepCache : 1;

    unsigned int flush : 1;

    unsigned int nonSeekAble : 1;

    unsigned int flockRelease : 1;

    unsigned int padding : 27;

    uint64_t fh;

    uint64_t lockOwner;
};

using CuckooFuseFiller = int (*)(void *, const char *, const struct stat *, off_t);

int CuckooMkdir(const std::string &path);

int CuckooCreate(const std::string &path, uint64_t &fd, int oflags, struct stat *stbuf);

int CuckooOpen(const std::string &path, int oflags, uint64_t &fd, struct stat *stbuf);

int CuckooUnlink(const std::string &path);

int CuckooOpenDir(const std::string &path, struct CuckooFuseInfo *fi);

int CuckooReadDir(const std::string &path, void *buf, CuckooFuseFiller filler, off_t offset, struct CuckooFuseInfo *fi);

int CuckooClose(const std::string &path, uint64_t fd, bool isFlush = false, int datasync = -1);

int CuckooGetStat(const std::string &path, struct stat *stbuf);

int CuckooCloseDir(uint64_t fd);

int CuckooInitWithZK(std::string zkEndPoint, const std::string &zkPath = "/cuckoo");

int CuckooInit(std::string &coordinatorIp, int coordinatorPort);

int CuckooDestroy();

int CuckooRmDir(const std::string &path);

int CuckooWrite(uint64_t fd, const std::string &path, const char *buffer, size_t size, off_t offset);

int CuckooRead(const std::string &path, uint64_t fd, char *buffer, size_t size, off_t offset);

int CuckooRename(const std::string &srcName, const std::string &dstName);

int CuckooFsync(const std::string &path, uint64_t fd, int datasync);

int CuckooStatFS(struct statvfs *vfsbuf);

int CuckooDeleteCache(const std::string &path);

int CuckooUtimens(const std::string &path, int64_t accessTime = -1, int64_t modifyTime = -1);

int CuckooChown(const std::string &path, uid_t uid, gid_t gid);

int CuckooChmod(const std::string &path, mode_t mode);

int CuckooTruncate(const std::string &path, off_t size);

int CuckooRenamePersist(const std::string &srcName, const std::string &dstName);
