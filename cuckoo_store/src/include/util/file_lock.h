/* Copyright (c) 2025 Huawei Technologies Co., Ltd.
 * SPDX-License-Identifier: MulanPSL-2.0
 */

#pragma once

#include <condition_variable>
#include <memory>
#include <shared_mutex>
#include <unordered_map>

#define FILE_BUCKET 100

enum class LockMode { X = -1, S = 1 };

struct FileLockState
{
    std::shared_ptr<std::condition_variable_any> cv;
    int lockCount;      // >0: S锁数量; <0: X锁; =0: 无锁
    int waitingThreads; // 等待此锁的线程数
};

class FileLock {
  public:
    void ReleaseFileLock(uint64_t inodeId, LockMode m);
    bool GetFileLock(uint64_t inodeId, LockMode m, bool wait = true);
    bool TryGetFileLock(uint64_t inodeId, LockMode m);
    void WaitGetFileLock(uint64_t inodeId, LockMode m);
    bool TestLocked(uint64_t inodeId, LockMode m = LockMode::S);

  private:
    bool innerGetFileLock(uint64_t inodeId, LockMode m, bool wait = true);
    std::unordered_map<uint64_t, FileLockState> inodeIdTofileLockStateMap;
    std::shared_mutex mutex;
};

class FileLocker {
  public:
    FileLocker(FileLock *initFileLock, uint64_t initInodeId, LockMode initLockMode, bool wait);
    ~FileLocker();
    bool isLocked();

    FileLock *fileLock;
    uint64_t inodeId;
    LockMode lockMode;
    bool locked = false;
};
