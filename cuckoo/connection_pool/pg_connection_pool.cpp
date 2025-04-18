/* Copyright (c) 2025 Huawei Technologies Co., Ltd.
 * SPDX-License-Identifier: MulanPSL-2.0
 */

#include "connection_pool/pg_connection_pool.h"

#include "connection_pool/pg_connection.h"

void PGConnectionPool::BackgroundPoolManager()
{
    while (working) {
        // 1. fetch conn
        PGConnection *conn = GetPGConnection();

        // 2. wait for command
        Task *taskToExec = nullptr;
        {
            std::unique_lock<std::mutex> lk(pendingTaskMutex);
            cvPendingTaskNotEmpty.wait(lk, [this]() -> bool { return !pendingTask.empty() || !working; });
            if (!working)
                break;

            // fetch command
            taskToExec = pendingTask.front();
            pendingTask.pop();
            for (int i = 0; i < TaskSupportBatchType::NOT_SUPPORT; ++i) {
                if (taskToExec != supportBatchTaskList[i].task)
                    continue;
                {
                    std::unique_lock<std::mutex> lk(supportBatchTaskList[i].taskMutex);
                    supportBatchTaskList[i].task = new Task(batchTaskBufferMaxSize);
                    supportBatchTaskList[i].task->isBatch = true;
                }
                supportBatchTaskList[i].cvBatchNotFull.notify_one();
                break;
            }
        }
        cvPendingTaskNotFull.notify_one();

        // 3. exec bt backgroundworker of connection
        conn->Exec(taskToExec);
    }
}

PGConnectionPool::PGConnectionPool(const uint16_t port,
                                   const char *userName,
                                   const int connPoolSize,
                                   const uint16_t pendingTaskBufferMaxSize,
                                   const uint16_t batchTaskBufferMaxSize)
{
    for (int i = 0; i < connPoolSize; ++i) {
        PGConnection *conn = new PGConnection(this, "127.0.0.1", port, userName);
        currentManagedConn.insert(conn);
        connPool.push(conn);
    }
    this->pendingTaskBufferMaxSize = pendingTaskBufferMaxSize;
    this->batchTaskBufferMaxSize = batchTaskBufferMaxSize;
    for (int i = 0; i < TaskSupportBatchType::NOT_SUPPORT; ++i) {
        supportBatchTaskList[i].task = new Task(batchTaskBufferMaxSize);
        supportBatchTaskList[i].task->isBatch = true;
    }

    working = true;
    backgroundPoolManager = std::thread(&PGConnectionPool::BackgroundPoolManager, this);
}

void PGConnectionPool::ReaddWorkingPGConnection(PGConnection *conn)
{
    {
        std::unique_lock<std::mutex> lk(connPoolMutex);
        connPool.push(conn);
    }
    cvPoolNotEmpty.notify_one();
}

PGConnection *PGConnectionPool::GetPGConnection()
{
    PGConnection *result = NULL;
    {
        std::unique_lock<std::mutex> lk(connPoolMutex);
        cvPoolNotEmpty.wait(lk, [this]() -> bool { return !this->connPool.empty(); });

        result = connPool.front();
        connPool.pop();
    }
    return result;
}

// lifetime of job must be longer than this function. it will be freed later
void PGConnectionPool::DispatchAsyncMetaServiceJob(cuckoo::meta_proto::AsyncMetaServiceJob *job)
{
    // we only allow batch with others if:
    // 1. explicit allow batch with others
    // 2. all of the operations have a same type
    // 3. operation type support batch
    const cuckoo::meta_proto::MetaRequest *request = job->GetRequest();
    if (request->type_size() == 0)
        return;
    cuckoo::meta_proto::MetaServiceType type = request->type(0);
    TaskSupportBatchType taskSupportBatchType = ConvertMetaServiceTypeToTaskSupportBatchType(type);
    bool allowBatchWithOthers =
        request->allow_batch_with_others() && taskSupportBatchType != TaskSupportBatchType::NOT_SUPPORT;
    if (allowBatchWithOthers) {
        for (int i = 1; i < request->type_size(); ++i)
            if (request->type(i) != type) {
                allowBatchWithOthers = false;
                break;
            }
    }

    Task *toInsertTask = NULL;
    if (allowBatchWithOthers) {
        std::unique_lock<std::mutex> lk(supportBatchTaskList[taskSupportBatchType].taskMutex);
        supportBatchTaskList[taskSupportBatchType].cvBatchNotFull.wait(lk, [this, taskSupportBatchType]() -> bool {
            return supportBatchTaskList[taskSupportBatchType].task->jobList.size() < batchTaskBufferMaxSize;
        });
        if (supportBatchTaskList[taskSupportBatchType].task->jobList.size() == 0)
            toInsertTask = supportBatchTaskList[taskSupportBatchType].task;
        supportBatchTaskList[taskSupportBatchType].task->jobList.emplace_back(job);
    } else {
        toInsertTask = new Task();
        toInsertTask->jobList.emplace_back(job);
    }

    if (toInsertTask != NULL) {
        {
            std::unique_lock<std::mutex> lk(pendingTaskMutex);
            cvPendingTaskNotFull.wait(lk, [this]() -> bool { return pendingTask.size() < pendingTaskBufferMaxSize; });
            pendingTask.push(toInsertTask);
        }
        cvPendingTaskNotEmpty.notify_one();
    }
}

void PGConnectionPool::Stop()
{
    working = false;
    cvPendingTaskNotEmpty.notify_one();
}

PGConnectionPool::~PGConnectionPool()
{
    Stop();
    for (auto it = currentManagedConn.begin(); it != currentManagedConn.end(); ++it) {
        (*it)->Stop();
    }
    for (auto it = currentManagedConn.begin(); it != currentManagedConn.end(); ++it) {
        delete (*it);
    }
    backgroundPoolManager.join();
    currentManagedConn.clear();
}