/* Copyright (c) 2025 Huawei Technologies Co., Ltd.
 * SPDX-License-Identifier: MulanPSL-2.0
 */

#include "util/file_lock.h"

#include "log/logging.h"

void FileLock::ReleaseFileLock(uint64_t inodeId, LockMode m)
{
    std::unique_lock<std::shared_mutex> lock(mutex);
    if (inodeIdTofileLockStateMap.find(inodeId) == inodeIdTofileLockStateMap.end()) {
        return;
    }
    /* file is locked, exists in map */
    std::shared_ptr<std::condition_variable_any> cv = inodeIdTofileLockStateMap[inodeId].cv;
    bool toNotify = false;
    if (m == LockMode::S) {
        if (--inodeIdTofileLockStateMap[inodeId].lockCount == 0) {
            toNotify = true;
        }
    } else {
        if (++inodeIdTofileLockStateMap[inodeId].lockCount == 0) {
            toNotify = true;
        }
    }
    if (toNotify) {
        if (inodeIdTofileLockStateMap[inodeId].waitingThreads == 0) {
            /* no one is currently waiting for this lock, able to erase */
            inodeIdTofileLockStateMap.erase(inodeId);
        } else {
            /* wake up all waiting for this lock, may be slocks */
            cv->notify_all();
        }
    }
}

bool FileLock::GetFileLock(uint64_t inodeId, LockMode m, bool wait) { return innerGetFileLock(inodeId, m, wait); }

bool FileLock::TryGetFileLock(uint64_t inodeId, LockMode m) { return innerGetFileLock(inodeId, m, false); }

void FileLock::WaitGetFileLock(uint64_t inodeId, LockMode m) { innerGetFileLock(inodeId, m, true); }

bool FileLock::innerGetFileLock(uint64_t inodeId, LockMode m, bool wait)
{
    std::unique_lock<std::shared_mutex> lock(mutex);

    /* do not insert */
    int lockCnt = 0;
    if (inodeIdTofileLockStateMap.count(inodeId) != 0) {
        lockCnt = inodeIdTofileLockStateMap[inodeId].lockCount;
    }

    if (lockCnt == 0) {
        /* not locked */
        if (inodeIdTofileLockStateMap[inodeId].waitingThreads == 0) {
            /* no one is waiting, create cv */
            inodeIdTofileLockStateMap[inodeId].cv = std::make_shared<std::condition_variable_any>();
        }
        inodeIdTofileLockStateMap[inodeId].lockCount = static_cast<int>(m);
    } else if (lockCnt > 0 && m == LockMode::S) {
        /* slocked, do not create new cv */
        inodeIdTofileLockStateMap[inodeId].lockCount++;
    } else if (wait) {
        /* xlocked, slocked before xlock, same cv for this inode */
        std::shared_ptr<std::condition_variable_any> cv = inodeIdTofileLockStateMap[inodeId].cv;
        if (cv == nullptr) {
            CUCKOO_LOG(LOG_ERROR) << "innerGetFileLock(): cv == nullptr for inode Id " << inodeId;
            return false;
        }
        /* increment lockWait */
        ++inodeIdTofileLockStateMap[inodeId].waitingThreads;
        if (m == LockMode::X) {
            cv->wait(lock, [this, inodeId]() { return inodeIdTofileLockStateMap[inodeId].lockCount == 0; });
        } else {
            cv->wait(lock, [this, inodeId]() { return inodeIdTofileLockStateMap[inodeId].lockCount >= 0; });
        }
        /* decrement lockWait */
        --inodeIdTofileLockStateMap[inodeId].waitingThreads;
        /* change lockCnt */
        inodeIdTofileLockStateMap[inodeId].lockCount += static_cast<int>(m);
    } else {
        /* try get lock failed */
        return false;
    }
    return true;
}

bool FileLock::TestLocked(uint64_t inodeId, LockMode m)
{
    std::shared_lock<std::shared_mutex> lock(mutex);
    if (m == LockMode::X) {
        /* no any lock, and no one waiting */
        return inodeIdTofileLockStateMap.count(inodeId) != 0;
    }
    /* no any lock, or xLocked */
    return inodeIdTofileLockStateMap.count(inodeId) != 0 && inodeIdTofileLockStateMap[inodeId].lockCount < 0;
}

/* -------------- locker class ------------------- */

FileLocker::FileLocker(FileLock *initFileLock, uint64_t initInodeId, LockMode initLockMode, bool wait)
{
    fileLock = initFileLock;
    inodeId = initInodeId;
    lockMode = initLockMode;
    locked = fileLock->GetFileLock(initInodeId, initLockMode, wait);
}

FileLocker::~FileLocker()
{
    if (locked) {
        fileLock->ReleaseFileLock(inodeId, lockMode);
    }
}

bool FileLocker::isLocked() { return locked; }
