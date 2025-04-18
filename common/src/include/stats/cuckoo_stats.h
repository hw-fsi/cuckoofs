/* Copyright (c) 2025 Huawei Technologies Co., Ltd.
 * SPDX-License-Identifier: MulanPSL-2.0
 */

#pragma once

#include <securec.h>
#include <atomic>
#include <chrono>
#include <cmath>
#include <stop_token>

enum {
    FUSE_OPS = 0,
    FUSE_LAT,
    FUSE_READ,
    FUSE_READ_OPS,
    FUSE_READ_LAT,
    FUSE_WRITE,
    FUSE_WRITE_OPS,
    FUSE_WRITE_LAT,
    META_OPS,
    META_LAT,
    META_OPEN,
    META_OPEN_ATOMIC,
    META_RELEASE,
    META_STAT,
    META_STAT_LAT,
    META_LOOKUP,
    META_CREATE,
    META_UNLINK,
    META_MKDIR,
    META_RMDIR,
    META_OPENDIR,
    META_READDIR,
    META_RENAME,
    META_ACCESS,
    META_RELEASEDIR,
    META_TRUNCATE,
    META_FLUSH,
    META_FSYNC,
    BLOCKCACHE_READ,
    BLOCKCACHE_WRITE,
    OBJ_GET,
    OBJ_PUT,
    STATS_END
};

class CuckooStats {
  public:
    static CuckooStats &GetInstance()
    {
        static CuckooStats instance;
        return instance;
    }
    CuckooStats()
    {
        for (auto &s : stats) {
            s.store(0);
        }
    }
    std::atomic<size_t> stats[STATS_END];
};

class StatFuseTimer {
  public:
    StatFuseTimer(int item = FUSE_LAT)
        : item(item)
    {
        start_time = std::chrono::steady_clock::now();
    }
    ~StatFuseTimer()
    {
        auto end_time = std::chrono::steady_clock::now();
        CuckooStats::GetInstance().stats[item] +=
            std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time).count();
    }
    std::chrono::steady_clock::time_point start_time;
    int item;
};

std::string formatU64(size_t size);
std::string formatOp(size_t size);
double formatTime(size_t mus, size_t ops);
void PrintStats(std::string_view mountPath, std::stop_token stoken);
