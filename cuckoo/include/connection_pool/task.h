/* Copyright (c) 2025 Huawei Technologies Co., Ltd.
 * SPDX-License-Identifier: MulanPSL-2.0
 */

#ifndef CUCKOO_POOLER_TASK_H
#define CUCKOO_POOLER_TASK_H

#include <brpc/server.h>
#include <vector>
#include "cuckoo_meta_rpc.pb.h"

namespace cuckoo::meta_proto
{

class AsyncMetaServiceJob {
  private:
    brpc::Controller *cntl;
    const MetaRequest *request;
    Empty *response;
    google::protobuf::Closure *done;

  public:
    AsyncMetaServiceJob(brpc::Controller *cntl,
                        const MetaRequest *request,
                        Empty *response,
                        google::protobuf::Closure *done)
        : cntl(cntl),
          request(request),
          response(response),
          done(done)
    {
    }
    brpc::Controller *GetCntl() { return cntl; }
    const MetaRequest *GetRequest() { return request; }
    Empty *GetResponse() { return response; }
    void Done() { done->Run(); }
};

} // namespace cuckoo::meta_proto

class Task {
  public:
    bool isBatch;
    std::vector<cuckoo::meta_proto::AsyncMetaServiceJob *> jobList;
    Task(int n)
    {
        isBatch = false;
        jobList.reserve(n);
    }
    Task() { isBatch = false; }
};

#endif