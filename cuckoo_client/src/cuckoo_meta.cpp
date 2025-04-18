/* Copyright (c) 2025 Huawei Technologies Co., Ltd.
 * SPDX-License-Identifier: MulanPSL-2.0
 */

#include "cuckoo_meta.h"

#include <atomic>
#include <condition_variable>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <mutex>
#include <unordered_map>

#include <sys/stat.h>
#include <sys/time.h>

#include "buffer/dir_open_instance.h"
#include "cm/cuckoo_cm.h"
#include "cuckoo_store/cuckoo_store.h"
#include "inner_cuckoo_meta.h"
#include "router.h"
#include "utils.h"

constexpr int FILE_NUMBER_PER_EPOCH = 1048576;
constexpr int FILE_NUMBER_PER_WORKER = 4096;

std::shared_ptr<Router> router;

int CuckooInit(std::string &coordinatorIp, int coordinatorPort)
{
    int ret = CuckooStore::GetInstance()->GetInitStatus();
    if (ret != SUCCESS) {
        return ret;
    }
    ServerIdentifier coordinator(coordinatorIp, coordinatorPort);
    router = std::make_shared<Router>(coordinator);
    return 0;
}

int CuckooInitWithZK(std::string zkEndPoint, const std::string &zkPath)
{
    int ret = CuckooCM::GetInstance(zkEndPoint, 10000, zkPath)->GetInitStatus();
    if (ret != SUCCESS) {
        return ret;
    }
    CuckooCM::GetInstance()->CheckMetaDataStatus();
    {
        std::mutex mu;
        std::unique_lock<std::mutex> lock(mu);
        CuckooCM::GetInstance()->GetMetaDataReadyCv().wait(lock, []() {
            return CuckooCM::GetInstance()->GetMetaDataStatus();
        });
    }
    ret = CuckooStore::GetInstance()->GetInitStatus();
    if (ret != SUCCESS) {
        return ret;
    }
    std::string coordinatorIp;
    int coordinatorPort = 0;
    ret = CuckooCM::GetInstance()->FetchCoordinatorInfo(coordinatorIp, coordinatorPort);
    if (ret != SUCCESS) {
        return ret;
    }
    ServerIdentifier coordinator(coordinatorIp, coordinatorPort);
    router = std::make_shared<Router>(coordinator);
    return 0;
}

int CuckooMkdir(const std::string &path)
{
    std::shared_ptr<Connection> conn = router->GetCoordinatorConn();
    if (!conn) {
        CUCKOO_LOG(LOG_ERROR) << "route error";
        return PROGRAM_ERROR;
    }

    int errorCode = conn->Mkdir(path.c_str());
#ifdef ZK_INIT
    int cnt = 0;
    while (cnt < RETRY_CNT && errorCode == SERVER_FAULT) {
        ++cnt;
        sleep(SLEEPTIME);
        conn = router->TryToUpdateCNConn(conn);
        errorCode = conn->Mkdir(path.c_str());
    }
#endif
    return errorCode;
}

int CuckooCreate(const std::string &path, uint64_t &fd, int oflags, struct stat *stbuf)
{
    std::shared_ptr<Connection> conn = router->GetWorkerConnByPath(path);
    if (!conn) {
        CUCKOO_LOG(LOG_ERROR) << "route error";
        return PROGRAM_ERROR;
    }
    uint64_t inodeId;
    int32_t nodeId;
    int errorCode = conn->Create(path.c_str(), inodeId, nodeId, stbuf);
#ifdef ZK_INIT
    int cnt = 0;
    while (cnt < RETRY_CNT && errorCode == SERVER_FAULT) {
        ++cnt;
        sleep(SLEEPTIME);
        conn = router->TryToUpdateWorkerConn(conn);
        errorCode = conn->Create(path.c_str(), inodeId, nodeId, stbuf);
    }
#endif
    if (errorCode == FILE_EXISTS && (oflags & O_EXCL))
        return FILE_EXISTS;

    fd = CuckooFd::GetInstance()->AttachFd(inodeId, oflags, nullptr, stbuf->st_size, path, nodeId);
    if (fd == UINT64_MAX) {
        CuckooFd::GetInstance()->DeleteOpenInstance(fd);
        return NOT_FOUND_FD;
    }
    return errorCode;
}

int CuckooGetStat(const std::string &path, struct stat *stbuf)
{
    std::shared_ptr<Connection> conn = router->GetWorkerConnByPath(path);
    if (!conn) {
        CUCKOO_LOG(LOG_ERROR) << "route error";
        return PROGRAM_ERROR;
    }
    int errorCode = conn->Stat(path.c_str(), stbuf);
#ifdef ZK_INIT
    int cnt = 0;
    while (cnt < RETRY_CNT && errorCode == SERVER_FAULT) {
        ++cnt;
        sleep(SLEEPTIME);
        conn = router->TryToUpdateWorkerConn(conn);
        errorCode = conn->Stat(path.c_str(), stbuf);
    }
#endif
    return errorCode;
}

int CuckooOpen(const std::string &path, int oflags, uint64_t &fd, struct stat *stbuf)
{
    std::shared_ptr<Connection> conn = router->GetWorkerConnByPath(path);
    if (!conn) {
        CUCKOO_LOG(LOG_ERROR) << "route error";
        return PROGRAM_ERROR;
    }

    std::shared_ptr<OpenInstance> openInstance = CuckooFd::GetInstance()->WaitGetNewOpenInstance();
    if (openInstance == nullptr) {
        CUCKOO_LOG(LOG_ERROR) << "new openInstance failed";
        return -ENOMEM;
    }
    uint64_t inodeId = 0;
    int64_t size = 0;
    int32_t nodeId = 0;
    int errorCode = conn->Open(path.c_str(), inodeId, size, nodeId, stbuf);
#ifdef ZK_INIT
    int cnt = 0;
    while (cnt < RETRY_CNT && errorCode == SERVER_FAULT) {
        ++cnt;
        sleep(SLEEPTIME);
        conn = router->TryToUpdateWorkerConn(conn);
        errorCode = conn->Open(path.c_str(), inodeId, size, nodeId, stbuf);
    }
#endif
    openInstance->inodeId = inodeId;
    openInstance->originalSize = size;
    openInstance->currentSize = size;
    openInstance->nodeId = nodeId;
    openInstance->path = path;
    openInstance->oflags = oflags;

    /******************* Fetch open meta finish ************************/

    /* allocate fd and handle the small file read */
    if (errorCode == SUCCESS) {
        if (openInstance->originalSize > 0 && openInstance->originalSize < READ_BIGFILE_SIZE &&
            (openInstance->oflags & O_ACCMODE) == O_RDONLY) {
            // For small files: read all when open
            std::shared_ptr<char> buffer;
            if (openInstance->oflags & __O_DIRECT) {
                int alignedNum = openInstance->originalSize / 512 + int(openInstance->originalSize % 512 != 0);
                buffer = std::shared_ptr<char>((char *)aligned_alloc(512, 512 * alignedNum), free);
            } else {
                buffer = std::shared_ptr<char>((char *)malloc(openInstance->originalSize), free);
            }
            if (buffer == nullptr) {
                CUCKOO_LOG(LOG_ERROR) << "In CuckooOpen() malloc failed";
                CuckooFd::GetInstance()->ReleaseOpenInstance();
                return -ENOMEM;
            }
            openInstance->readBuffer = buffer;
            openInstance->readBufferSize = openInstance->originalSize;
            int ret = InnerCuckooReadSmallFiles(openInstance.get());
            if (ret < 0) {
                CuckooFd::GetInstance()->ReleaseOpenInstance();
                return ret;
            }
        }
        fd = CuckooFd::GetInstance()->AttachFd(path, openInstance);
    }
    return errorCode;
}

int CuckooClose(const std::string &path, uint64_t fd, bool isFlush, int datasync)
{
    OpenInstance *openInstance = CuckooFd::GetInstance()->GetOpenInstanceByFd(fd).get();
    if (openInstance == nullptr) {
        CUCKOO_LOG(LOG_ERROR) << "In CuckooClose(): fd not found for openInstance";
        return NOT_FOUND_FD;
    }

    size_t size = openInstance->currentSize;
    // only read small files does not open file
    if (openInstance->isOpened) {
        int innerRet = InnerCuckooTmpClose(openInstance, isFlush, datasync >= 0); // here may fail, mark in writeFail
        if (innerRet != 0) {
            if (!isFlush) {
                CuckooFd::GetInstance()->DeleteOpenInstance(fd);
            }
            return innerRet;
        }
    }
    /* update only once if truncate */
    if (openInstance->readFail || openInstance->writeFail || datasync > 0 ||
        (!openInstance->nodeFail && size == openInstance->originalSize)) {
        bool readFail = openInstance->readFail;
        if (!isFlush) {
            CuckooFd::GetInstance()->DeleteOpenInstance(fd);
        }
        if (readFail) {
            return -EIO;
        }
        return SUCCESS;
    }

    std::shared_ptr<Connection> conn = router->GetWorkerConnByPath(path);
    if (!conn) {
        CUCKOO_LOG(LOG_ERROR) << "route error";
        return PROGRAM_ERROR;
    }

    int errorCode = conn->Close(path.c_str(), size, 0, openInstance->nodeId);
#ifdef ZK_INIT
    int cnt = 0;
    while (cnt < RETRY_CNT && errorCode == SERVER_FAULT) {
        ++cnt;
        sleep(SLEEPTIME);
        conn = router->TryToUpdateWorkerConn(conn);
        errorCode = conn->Close(path.c_str(), size, 0, openInstance->nodeId);
    }
#endif
    openInstance->originalSize = size;
    if (!isFlush) {
        CuckooFd::GetInstance()->DeleteOpenInstance(fd);
    }
    return errorCode;
}

int CuckooUnlink(const std::string &path)
{
    std::shared_ptr<Connection> conn = router->GetWorkerConnByPath(path);
    if (!conn) {
        CUCKOO_LOG(LOG_ERROR) << "route error";
        return PROGRAM_ERROR;
    }

    uint64_t inodeId = 0;
    int64_t size = 0;
    int32_t nodeId = 0;
    int errorCode = conn->Unlink(path.c_str(), inodeId, size, nodeId);
#ifdef ZK_INIT
    int cnt = 0;
    while (cnt < RETRY_CNT && errorCode == SERVER_FAULT) {
        ++cnt;
        sleep(SLEEPTIME);
        conn = router->TryToUpdateWorkerConn(conn);
        errorCode = conn->Unlink(path.c_str(), inodeId, size, nodeId);
    }
#endif
    int ret = 0;
    if (errorCode == SUCCESS) {
        // delete data
        ret = InnerCuckooUnlink(inodeId, nodeId, path);
        if (ret != 0) {
            CUCKOO_LOG(LOG_ERROR) << "In CuckooUnlink(): delete cache " << path << " failed";
        }
    }

    return errorCode;
}

int CuckooReadDir(const std::string &path, void *buf, CuckooFuseFiller filler, off_t offset, struct CuckooFuseInfo *fi)
{
    uint64_t fd = fi->fh;
    int idx = offset;
    std::unordered_map<std::string, std::shared_ptr<Connection>> workerInfo;
    int ret = SUCCESS;

    DirOpenInstance *dirOpenInstance = CuckooFd::GetInstance()->GetDirOpenInstanceByFd(fd);
    if (dirOpenInstance == nullptr) {
        CUCKOO_LOG(LOG_ERROR) << "In CuckooReadDir(): fd not found for dirOpenInstance";
        return NOT_FOUND_FD;
    }
    if (offset == 0) {
        /* offset == 0 means that the readdir function is the first called, we should prepare the connection to all dn
         */
        ret = router->GetAllWorkerConnection(workerInfo);

        if (ret != SUCCESS) {
            return GET_ALL_WORKER_CONN_FAILED;
        }
        dirOpenInstance->SetAllWorkerInfo(workerInfo);
        idx = 1;
        filler(buf, ".", nullptr, idx++);
        filler(buf, "..", nullptr, idx++);
    }

    int workerNotFinished = dirOpenInstance->workingWorkers.size();
    if (dirOpenInstance->offset >= dirOpenInstance->partialEntryVec.size() && workerNotFinished != 0) {
        uint32_t fileNumberPerWorker = std::min(FILE_NUMBER_PER_EPOCH / workerNotFinished, FILE_NUMBER_PER_WORKER);
        dirOpenInstance->workers.clear();
        dirOpenInstance->workers = dirOpenInstance->workingWorkers;
        dirOpenInstance->workingWorkers.clear();
        dirOpenInstance->partialEntryVec.clear();
        dirOpenInstance->offset = 0;
        for (auto it = dirOpenInstance->workers.begin(); it != dirOpenInstance->workers.end(); ++it) {
            std::string ipPort = it->first;
            std::shared_ptr<Connection> conn = it->second;
            Connection::ReadDirResponse readDirResponse;
            ret = conn->ReadDir(path.c_str(),
                                readDirResponse,
                                fileNumberPerWorker,
                                dirOpenInstance->lastShardIndexes[ipPort],
                                dirOpenInstance->lastFileNames[ipPort].empty()
                                    ? nullptr
                                    : dirOpenInstance->lastFileNames[ipPort].c_str());
#ifdef ZK_INIT
            int cnt = 0;
            while (cnt < RETRY_CNT && ret == SERVER_FAULT) {
                ++cnt;
                sleep(SLEEPTIME);
                conn = router->TryToUpdateWorkerConn(conn);
                ret = conn->ReadDir(path.c_str(),
                                    readDirResponse,
                                    fileNumberPerWorker,
                                    dirOpenInstance->lastShardIndexes[ipPort],
                                    dirOpenInstance->lastFileNames[ipPort].empty()
                                        ? nullptr
                                        : dirOpenInstance->lastFileNames[ipPort].c_str());
            }
#endif
            dirOpenInstance->lastShardIndexes[ipPort] = readDirResponse.response->last_shard_index();
            if (readDirResponse.response->last_file_name() == nullptr)
                dirOpenInstance->lastFileNames[ipPort] = "";
            else
                dirOpenInstance->lastFileNames[ipPort] = readDirResponse.response->last_file_name()->str();
            auto result_list = readDirResponse.response->result_list();

            // fill the fuse readdir buffer using metadata
            for (unsigned i = 0; i < result_list->size(); i++) {
                dirOpenInstance->partialEntryVec.push_back(result_list->Get(i)->file_name()->c_str());
                dirOpenInstance->fileModes.push_back(result_list->Get(i)->st_mode());
            }
            if (result_list->size() < fileNumberPerWorker && ret == SUCCESS) {
                dirOpenInstance->lastFileNames.erase(ipPort);
            } else {
                dirOpenInstance->workingWorkers.emplace(ipPort, conn);
            }
        }
    }
    for (size_t i = dirOpenInstance->offset; i < dirOpenInstance->partialEntryVec.size(); i++) {
        struct stat st;
        errno_t err = memset_s(&st, sizeof(st), 0, sizeof(st));
        if (err != 0) {
            CUCKOO_LOG(LOG_ERROR) << "Secure func failed: " << err;
            return PROGRAM_ERROR;
        }

        st.st_mode = static_cast<mode_t>(dirOpenInstance->fileModes[i]);
        if (filler(buf, dirOpenInstance->partialEntryVec[i].c_str(), &st, idx++)) {
            dirOpenInstance->offset = i;
            return 0;
        }
    }
    dirOpenInstance->offset = dirOpenInstance->partialEntryVec.size();
    return ret;
}

int CuckooOpenDir(const std::string &path, struct CuckooFuseInfo *fi)
{
    std::shared_ptr<Connection> conn = router->GetCoordinatorConn();
    if (!conn) {
        CUCKOO_LOG(LOG_ERROR) << "route error";
        return PROGRAM_ERROR;
    }
    uint64_t inodeId = 0;
    int errorCode = conn->OpenDir(path.c_str(), inodeId);
#ifdef ZK_INIT
    int cnt = 0;
    while (cnt < RETRY_CNT && errorCode == SERVER_FAULT) {
        ++cnt;
        sleep(SLEEPTIME);
        conn = router->TryToUpdateCNConn(conn);
        errorCode = conn->OpenDir(path.c_str(), inodeId);
    }
#endif

    if (errorCode == 0) {
        uint64_t fd = 0;
        fd = CuckooFd::GetInstance()->AttachDirFd(errorCode);
        fi->fh = fd;
    }
    return errorCode;
}

int CuckooCloseDir(uint64_t fd)
{
    DirOpenInstance *dirOpenInstance = CuckooFd::GetInstance()->GetDirOpenInstanceByFd(fd);
    if (dirOpenInstance == nullptr) {
        CUCKOO_LOG(LOG_ERROR) << "In CuckooCloseDir(): fd not found for dirOpenInstance";
        return NOT_FOUND_FD;
    }

    int delRet = CuckooFd::GetInstance()->DeleteDirOpenInstance(fd);
    return delRet;
}

int CuckooDestroy()
{
    CuckooStore::GetInstance()->DeleteInstance();

    return 0;
}

int CuckooRmDir(const std::string &path)
{
    std::shared_ptr<Connection> conn = router->GetCoordinatorConn();
    if (!conn) {
        CUCKOO_LOG(LOG_ERROR) << "route error";
        return PROGRAM_ERROR;
    }
    int errorCode = conn->Rmdir(path.c_str());
#ifdef ZK_INIT
    int cnt = 0;
    while (cnt < RETRY_CNT && errorCode == SERVER_FAULT) {
        ++cnt;
        sleep(SLEEPTIME);
        conn = router->TryToUpdateCNConn(conn);
        errorCode = conn->Rmdir(path.c_str());
    }
#endif

    return errorCode;
}

int CuckooWrite(uint64_t fd, const std::string & /*path*/, const char *buffer, size_t size, off_t offset)
{
    std::shared_ptr<OpenInstance> openInstance = CuckooFd::GetInstance()->GetOpenInstanceByFd(fd);
    if (openInstance == nullptr) {
        CUCKOO_LOG(LOG_ERROR) << "In CuckooWrite(): fd not found for openInstance";
        return NOT_FOUND_FD;
    }

    openInstance->writeCnt++;
    int ret = InnerCuckooWrite(openInstance.get(), buffer, size, offset);
    return ret;
}

int CuckooRead(const std::string & /*path*/, uint64_t fd, char *buffer, size_t size, off_t offset)
{
    std::shared_ptr<OpenInstance> openInstance = CuckooFd::GetInstance()->GetOpenInstanceByFd(fd);
    if (openInstance == nullptr) {
        CUCKOO_LOG(LOG_ERROR) << "In CuckooRead(): fd not found for openInstance";
        return NOT_FOUND_FD;
    }

    int ret = InnerCuckooRead(openInstance.get(), buffer, size, offset);
    if (ret < 0) {
        openInstance->readFail = true;
    }
    return ret;
}

int CuckooRename(const std::string &srcName, const std::string &dstName)
{
    std::shared_ptr<Connection> conn = router->GetCoordinatorConn();
    if (!conn) {
        CUCKOO_LOG(LOG_ERROR) << "route error";
        return PROGRAM_ERROR;
    }
    int errorCode = conn->Rename(srcName.c_str(), dstName.c_str());
#ifdef ZK_INIT
    int cnt = 0;
    while (cnt < RETRY_CNT && errorCode == SERVER_FAULT) {
        ++cnt;
        sleep(SLEEPTIME);
        conn = router->TryToUpdateCNConn(conn);
        errorCode = conn->Rename(srcName.c_str(), dstName.c_str());
    }
#endif
    return errorCode;
}

int CuckooRenamePersist(const std::string &srcName, const std::string &dstName)
{
    struct stat stbuf;
    errno_t err = memset_s(&stbuf, sizeof(stbuf), 0, sizeof(stbuf));
    if (err != 0) {
        CUCKOO_LOG(LOG_ERROR) << "Secure func failed: " << err;
        return PROGRAM_ERROR;
    }

    int ret = 0;
    ret = CuckooGetStat(srcName, &stbuf);
    if (ret != 0) {
        return ret;
    }
    if (!S_ISREG(stbuf.st_mode)) {
        // we do not support rename directory
        return -EOPNOTSUPP;
    }
    // first copy the data in obs
    ret = InnerCuckooCopydata(srcName, dstName);
    if (ret != 0) {
        return ret;
    }
    // update the metadata for rename
    std::shared_ptr<Connection> conn = router->GetCoordinatorConn();
    if (!conn) {
        CUCKOO_LOG(LOG_ERROR) << "route error";
        return PROGRAM_ERROR;
    }
    int errorCode = conn->Rename(srcName.c_str(), dstName.c_str());
#ifdef ZK_INIT
    int cnt = 0;
    while (cnt < RETRY_CNT && errorCode == SERVER_FAULT) {
        ++cnt;
        sleep(SLEEPTIME);
        conn = router->TryToUpdateCNConn(conn);
        errorCode = conn->Rename(srcName.c_str(), dstName.c_str());
    }
#endif
    if (errorCode == SUCCESS) {
        // delete src object
        InnerCuckooDeleteDataAfterRename(srcName);
    } else {
        // delete dst object
        InnerCuckooDeleteDataAfterRename(dstName);
    }
    return errorCode;
}

int CuckooFsync(const std::string &path, uint64_t fd, int datasync)
{
    return CuckooClose(path, fd, true, datasync == 0 ? 0 : 1);
}

int CuckooStatFS(struct statvfs *vfsbuf)
{
    int ret = InnerCuckooStatFS(vfsbuf);
    return ret;
}

int CuckooUtimens(const std::string &path, int64_t accessTime, int64_t modifyTime)
{
    std::shared_ptr<Connection> conn = router->GetWorkerConnByPath(path);
    if (!conn) {
        CUCKOO_LOG(LOG_ERROR) << "route error";
        return PROGRAM_ERROR;
    }

    int errorCode = conn->UtimeNs(path.c_str(), accessTime, modifyTime);
#ifdef ZK_INIT
    int cnt = 0;
    while (cnt < RETRY_CNT && errorCode == SERVER_FAULT) {
        ++cnt;
        sleep(SLEEPTIME);
        conn = router->TryToUpdateWorkerConn(conn);
        errorCode = conn->UtimeNs(path.c_str(), accessTime, modifyTime);
    }
#endif
    return errorCode;
}

int CuckooChown(const std::string &path, uid_t uid, gid_t gid)
{
    std::shared_ptr<Connection> conn = router->GetWorkerConnByPath(path);
    if (!conn) {
        CUCKOO_LOG(LOG_ERROR) << "route error";
        return PROGRAM_ERROR;
    }

    int errorCode = conn->Chown(path.c_str(), uid, gid);
#ifdef ZK_INIT
    int cnt = 0;
    while (cnt < RETRY_CNT && errorCode == SERVER_FAULT) {
        ++cnt;
        sleep(SLEEPTIME);
        conn = router->TryToUpdateWorkerConn(conn);
        errorCode = conn->Chown(path.c_str(), uid, gid);
    }
#endif
    return errorCode;
}

int CuckooChmod(const std::string &path, mode_t mode)
{
    std::shared_ptr<Connection> conn = router->GetWorkerConnByPath(path);
    if (!conn) {
        CUCKOO_LOG(LOG_ERROR) << "route error";
        return PROGRAM_ERROR;
    }

    int errorCode = conn->Chmod(path.c_str(), mode);
#ifdef ZK_INIT
    int cnt = 0;
    while (cnt < RETRY_CNT && errorCode == SERVER_FAULT) {
        ++cnt;
        sleep(SLEEPTIME);
        conn = router->TryToUpdateWorkerConn(conn);
        errorCode = conn->Chmod(path.c_str(), mode);
    }
#endif
    return errorCode;
}

// User shouldn't cmake concurrent truncate and open
int CuckooTruncate(const std::string &path, off_t size)
{
    int ret = 0;
    uint64_t inodeId = 0;

    int oflags = O_WRONLY;
    uint64_t fd = 0;

    struct stat st;
    memset(&st, 0, sizeof(st));
    ret = CuckooOpen(path, oflags, fd, &st);
    if (ret != 0) {
        CuckooClose(path, fd, true, -1);
        CuckooClose(path, fd, false, -1);
        return ret;
    }
    std::shared_ptr<OpenInstance> openInstance = CuckooFd::GetInstance()->GetOpenInstanceByFd(fd);
    inodeId = openInstance->inodeId;
    auto originalSize = openInstance->originalSize;

    // truncate the concurrent openInstances, update the size
    auto openInstanceSet = CuckooFd::GetInstance()->GetInodetoOpenInstanceSet(inodeId);
    for (auto &openInstance : openInstanceSet) {
        ret = InnerCuckooTruncateOpenInstance(openInstance.get(), size);
    }

    // truncate the cache file, which may not exist. Must called after TruncateOpenInstance, for size and obs update
    openInstance->writeCnt++;
    openInstance->originalSize = originalSize;

    ret = InnerCuckooTruncateFile(openInstance.get(), size);
    if (ret != 0) {
        openInstance->writeFail = true;
        CUCKOO_LOG(LOG_ERROR) << "truncateFile failed, ret = " << ret;
    }

    // flush and release must called to flush obs, update diskCache, delete openInstance, and update meta
    int flushRet = CuckooClose(path, fd, true, -1);
    if (flushRet != 0) {
        ret = ret == 0 ? flushRet : ret;
        CUCKOO_LOG(LOG_ERROR) << "Truncate Flush File failed, ret = " << ret;
    }
    int releaseRet = CuckooClose(path, fd, false, -1);
    if (releaseRet != 0) {
        ret = ret == 0 ? releaseRet : ret;
        CUCKOO_LOG(LOG_ERROR) << "Truncate Release File failed, ret = " << ret;
    }
    if (ret != 0) {
        return ret;
    }

    return ret;
}
