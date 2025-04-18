/* Copyright (c) 2025 Huawei Technologies Co., Ltd.
 * SPDX-License-Identifier: MulanPSL-2.0
 */

#include "buffer/dir_open_instance.h"

#include <cassert>

CuckooFd *CuckooFd::GetInstance()
{
    static CuckooFd m_singleton;
    return &m_singleton;
}

uint64_t CuckooFd::ObtainFd()
{
    uint64_t newFd = nextFD++;
    if (newFd <= 2 || newFd == UINT64_MAX) {
        CUCKOO_LOG(LOG_ERROR) << "ObtainFd(): fd restarts allocate";
        return ObtainFd();
    }
    return newFd;
}

void CuckooFd::AddOpenInstance(uint64_t fd, std::shared_ptr<OpenInstance> openInstance)
{
    std::unique_lock lock(openInstanceMapMutex);
    if (auto [it, inserted] = openInstanceMap.try_emplace(fd, std::move(openInstance)); !inserted) {
        CUCKOO_LOG(LOG_ERROR) << "AddOpenInstance(): fd" << fd << " already exists";
    }
}

int CuckooFd::DeleteOpenInstance(uint64_t fd, bool subCnt)
{
    int ret = -EBADF;
    std::unique_lock<std::shared_mutex> openInstanceMapLock(openInstanceMapMutex);
    auto it = openInstanceMap.find(fd);
    if (it != openInstanceMap.end()) {
        auto openInstance = it->second;
        openInstanceMap.erase(it);
        openInstanceMapLock.unlock();
        std::unique_lock<std::shared_mutex> inodeToOpenInstanceLock(inodeToOpenInstanceMutex);
        auto inodeId = openInstance->inodeId;
        inodeToOpenInstanceMap[inodeId].erase(openInstance);
        if (inodeToOpenInstanceMap[inodeId].empty()) {
            inodeToOpenInstanceMap.erase(inodeId);
        }
        inodeToOpenInstanceLock.unlock();
        if (subCnt) {
            ReleaseOpenInstance();
        }
        ret = 0;
    }
    return ret;
}

uint64_t CuckooFd::AttachFd(uint64_t inodeId,
                            int oflags,
                            std::shared_ptr<char> readBuffer,
                            uint64_t size,
                            std::string path,
                            int nodeId,
                            int backupNodeId,
                            bool isRead)
{
    std::shared_ptr<OpenInstance> openInstance = WaitGetNewOpenInstance();
    if (openInstance == nullptr) {
        return UINT64_MAX;
    }
    uint64_t fd = ObtainFd();
    if (isRead) {
        openInstance->readBuffer = readBuffer;
    }
    openInstance->inodeId = inodeId;
    openInstance->fd = fd;
    openInstance->physicalFd = UINT64_MAX;
    openInstance->oflags = oflags;
    openInstance->originalSize = size;
    openInstance->currentSize = size;
    openInstance->nodeId = nodeId;
    openInstance->backupNodeId = backupNodeId;
    openInstance->path = path;
    AddOpenInstance(fd, openInstance);
    std::unique_lock<std::shared_mutex> inodeToOpenInstanceLock(inodeToOpenInstanceMutex);
    inodeToOpenInstanceMap[inodeId].insert(openInstance);
    return fd;
}

uint64_t CuckooFd::AttachFd(const std::string & /*path*/, std::shared_ptr<OpenInstance> openInstance)
{
    uint64_t fd = ObtainFd();
    openInstance->fd = fd;
    AddOpenInstance(fd, openInstance);
    std::unique_lock<std::shared_mutex> inodeToOpenInstanceLock(inodeToOpenInstanceMutex);
    inodeToOpenInstanceMap[openInstance->inodeId].insert(openInstance);
    return fd;
}

std::shared_ptr<OpenInstance> CuckooFd::GetOpenInstanceByFd(uint64_t fd)
{
    std::shared_lock<std::shared_mutex> openInstanceMapLock(openInstanceMapMutex);
    if (openInstanceMap.contains(fd)) {
        return openInstanceMap[fd];
    }
    CUCKOO_LOG(LOG_ERROR) << "GetOpenInstanceByFd(): fd" << fd << " not found";
    return nullptr;
}

int CuckooFd::AddDirOpenInstance(uint64_t fd, DirOpenInstance *dirOpenInstance)
{
    std::unique_lock<std::shared_mutex> dirOpenInstanceLock(dirOpenInstanceMutex);
    if (!dirOpenInstanceMap.contains(fd)) {
        dirOpenInstanceMap[fd] = dirOpenInstance;
        return 0;
    }
    return -EBADF;
}

uint64_t CuckooFd::AttachDirFd(uint64_t /*inodeId*/)
{
    uint64_t fd = ObtainFd();
    auto *dirOpenInstance = new (std::nothrow) DirOpenInstance(fd);
    if (dirOpenInstance == nullptr) {
        return UINT64_MAX;
    }
    AddDirOpenInstance(fd, dirOpenInstance);
    return fd;
}

DirOpenInstance *CuckooFd::GetDirOpenInstanceByFd(uint64_t fd)
{
    std::shared_lock<std::shared_mutex> dirOpenInstanceLock(dirOpenInstanceMutex);
    return dirOpenInstanceMap.contains(fd) ? dirOpenInstanceMap[fd] : nullptr;
}

int CuckooFd::DeleteDirOpenInstance(uint64_t fd)
{
    int ret = -EBADF;
    std::unique_lock<std::shared_mutex> dirOpenInstanceLock(dirOpenInstanceMutex);
    auto it = dirOpenInstanceMap.find(fd);
    if (it != dirOpenInstanceMap.end()) {
        DirOpenInstance *dirOpenInstance = it->second;
        dirOpenInstanceMap.erase(it);
        dirOpenInstanceLock.unlock();
        delete dirOpenInstance;
        ret = 0;
    }
    return ret;
}

std::shared_ptr<OpenInstance> CuckooFd::WaitGetNewOpenInstance(bool addCnt)
{
    if (addCnt) {
        std::mutex mu;
        std::unique_lock<std::mutex> sleepLock(mu);
        newOpenInstanceCV.wait(sleepLock, [this]() { return currOpenInstance.load() < MAX_OPENINSTANCE_NUM; });
    }

    std::shared_ptr<OpenInstance> instance;
    try {
        instance = std::make_shared<OpenInstance>();
    } catch (const std::bad_alloc &) {
        instance = nullptr;
    }
    if (instance && addCnt) {
        ++currOpenInstance;
    }

    return instance;
}

void CuckooFd::ReleaseOpenInstance()
{
    --currOpenInstance;
    newOpenInstanceCV.notify_one();
}

std::unordered_set<std::shared_ptr<OpenInstance>> CuckooFd::GetInodetoOpenInstanceSet(uint64_t inodeId)
{
    std::shared_lock<std::shared_mutex> lock(inodeToOpenInstanceMutex);
    return inodeToOpenInstanceMap.contains(inodeId) ? inodeToOpenInstanceMap.at(inodeId)
                                                    : std::unordered_set<std::shared_ptr<OpenInstance>>{};
}
