/* Copyright (c) 2025 Huawei Technologies Co., Ltd.
 * SPDX-License-Identifier: MulanPSL-2.0
 */

#ifndef CUCKOO_CONNECTION_POOL_PG_CONNECTION_POOL_H
#define CUCKOO_CONNECTION_POOL_PG_CONNECTION_POOL_H

#include <condition_variable>
#include <queue>
#include <string>
#include <thread>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include "connection_pool/task.h"

class PGConnection;

class PGConnectionPool {
  private:
    std::unordered_set<PGConnection *> currentManagedConn;

    bool working;

    std::queue<PGConnection *> connPool;
    std::mutex connPoolMutex;
    std::condition_variable cvPoolNotEmpty;

    std::queue<Task *> pendingTask;
    std::mutex pendingTaskMutex;
    std::condition_variable cvPendingTaskNotEmpty;
    std::condition_variable cvPendingTaskNotFull;
    uint16_t pendingTaskBufferMaxSize;

    enum TaskSupportBatchType { MKDIR = 0, CREATE, STAT, UNLINK, OPEN, CLOSE, NOT_SUPPORT };
    TaskSupportBatchType ConvertMetaServiceTypeToTaskSupportBatchType(const cuckoo::meta_proto::MetaServiceType type)
    {
        switch (type) {
        case cuckoo::meta_proto::MetaServiceType::MKDIR:
            return TaskSupportBatchType::MKDIR;
        case cuckoo::meta_proto::MetaServiceType::CREATE:
            return TaskSupportBatchType::CREATE;
        case cuckoo::meta_proto::MetaServiceType::STAT:
            return TaskSupportBatchType::STAT;
        case cuckoo::meta_proto::MetaServiceType::UNLINK:
            return TaskSupportBatchType::UNLINK;
        case cuckoo::meta_proto::MetaServiceType::OPEN:
            return TaskSupportBatchType::OPEN;
        case cuckoo::meta_proto::MetaServiceType::CLOSE:
            return TaskSupportBatchType::CLOSE;
        default:
            return TaskSupportBatchType::NOT_SUPPORT;
        }
    }
    class TaskSupportBatch {
      public:
        Task *task;
        std::mutex taskMutex;
        std::condition_variable cvBatchNotFull;
    };
    TaskSupportBatch supportBatchTaskList[TaskSupportBatchType::NOT_SUPPORT];
    uint16_t batchTaskBufferMaxSize;

    std::thread backgroundPoolManager;

    PGConnection *GetPGConnection();
    void BackgroundPoolManager();

  public:
    PGConnectionPool(const uint16_t port,
                     const char *userName,
                     const int connPoolSize,
                     const uint16_t pendingTaskBufferMaxSize,
                     const uint16_t batchTaskBufferMaxSize);
    ~PGConnectionPool();

    void ReaddWorkingPGConnection(PGConnection *conn);

    void DispatchAsyncMetaServiceJob(cuckoo::meta_proto::AsyncMetaServiceJob *job);

    void Stop();
};

#endif