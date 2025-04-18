/* Copyright (c) 2025 Huawei Technologies Co., Ltd.
 * SPDX-License-Identifier: MulanPSL-2.0
 */

#include "transaction/cuckoo_distributed_transaction.h"

#include "catalog/pg_namespace_d.h"
#include "utils/lsyscache.h"

#include "utils/utils.h"

const char *DistributedTransactionTableName = "cuckoo_distributed_transaction";
const char *DistributedTransactionTableIndexName = "cuckoo_distributed_transaction_index";

Oid CuckooDistributedTransactionRelationId(void)
{
    GetRelationOid(DistributedTransactionTableName,
                         &CachedRelationOid[CACHED_RELATION_DISTRIBUTED_TRANSACTION_TABLE]);
    return CachedRelationOid[CACHED_RELATION_DISTRIBUTED_TRANSACTION_TABLE];
}

Oid CuckooDistributedTransactionRelationIndexId(void)
{
    GetRelationOid(DistributedTransactionTableIndexName,
                         &CachedRelationOid[CACHED_RELATION_DISTRIBUTED_TRANSACTION_TABLE_INDEX]);
    return CachedRelationOid[CACHED_RELATION_DISTRIBUTED_TRANSACTION_TABLE_INDEX];
}