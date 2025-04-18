/* Copyright (c) 2025 Huawei Technologies Co., Ltd.
 * SPDX-License-Identifier: MulanPSL-2.0
 */

#ifndef CUCKOO_CONNECTION_POOL_CUCKOO_META_RPC_H
#define CUCKOO_CONNECTION_POOL_CUCKOO_META_RPC_H

#include <brpc/server.h>
#include <butil/iobuf.h>
#include "connection_pool/pg_connection_pool.h"
#include "connection_pool/task.h"
#include "cuckoo_meta_rpc.pb.h"

namespace cuckoo::meta_proto
{

class MetaServiceImpl : public MetaService {
  private:
    std::shared_ptr<PGConnectionPool> pgConnectionPool;

  public:
    MetaServiceImpl(std::shared_ptr<PGConnectionPool> pgConnectionPool)
        : pgConnectionPool(pgConnectionPool)
    {
    }
    virtual ~MetaServiceImpl() {}

    virtual void MetaCall(google::protobuf::RpcController *cntlBase,
                          const MetaRequest *request,
                          Empty *response,
                          google::protobuf::Closure *done);
};

} // namespace cuckoo::meta_proto

#endif