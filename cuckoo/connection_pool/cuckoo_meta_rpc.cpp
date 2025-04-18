/* Copyright (c) 2025 Huawei Technologies Co., Ltd.
 * SPDX-License-Identifier: MulanPSL-2.0
 */

#include "connection_pool/cuckoo_meta_rpc.h"

#include <brpc/server.h>
#include <butil/iobuf.h>

namespace cuckoo::meta_proto
{

void MetaServiceImpl::MetaCall(google::protobuf::RpcController *cntlBase,
                               const MetaRequest *request,
                               Empty *response,
                               google::protobuf::Closure *done)
{
    brpc::ClosureGuard doneGuard(done);
    brpc::Controller *cntl = static_cast<brpc::Controller *>(cntlBase);

    AsyncMetaServiceJob *job = new AsyncMetaServiceJob(cntl, request, response, done);
    pgConnectionPool->DispatchAsyncMetaServiceJob(job);
    doneGuard.release();
}

} // namespace cuckoo::meta_proto