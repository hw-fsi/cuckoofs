/* Copyright (c) 2025 Huawei Technologies Co., Ltd.
 * SPDX-License-Identifier: MulanPSL-2.0
 */

#ifndef CUCKOO_POOLER_PG_CONNECTION_H
#define CUCKOO_POOLER_PG_CONNECTION_H

#include <flatbuffers/flatbuffers.h>
#include <condition_variable>
#include <mutex>
#include <queue>
#include <string>
#include <thread>
#include <vector>
#include "connection_pool/pg_connection_pool.h"
#include "libpq-fe.h"
#include "remote_connection_utils/serialized_data.h"

class PGConnectionPool;

class PGConnection {
  private:
    bool working;

    PGConnectionPool *parent;

    flatbuffers::FlatBufferBuilder flatBufferBuilder;
    SerializedData replyBuilder;

    Task *taskToExec;

    std::mutex execMutex;
    std::condition_variable cvExecing;
    std::thread thread;

  public:
    PGconn *conn;

    PGConnection(PGConnectionPool *parent, const char *ip, const int port, const char *userName);
    ~PGConnection();

    void BackgroundWorker();

    void Exec(Task *taskToExec);

    void Stop();
};

#endif