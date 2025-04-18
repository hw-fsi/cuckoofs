/* Copyright (c) 2025 Huawei Technologies Co., Ltd.
 * SPDX-License-Identifier: MulanPSL-2.0
 */

#ifndef CUCKOO_DISTRIBUTED_TRANSACTION_H
#define CUCKOO_DISTRIBUTED_TRANSACTION_H

#include "postgres.h"

typedef struct FormData_cuckoo_distributed_transaction 
{
    int nodeId;
    text gid;
} FormData_cuckoo_distributed_transaction;
typedef FormData_cuckoo_distributed_transaction* Form_cuckoo_distributed_transaction;

#define Anum_cuckoo_distributed_transaction_nodeid 1
#define Anum_cuckoo_distributed_transaction_gid 2
#define Natts_cuckoo_distributed_transaction 2

Oid CuckooDistributedTransactionRelationId(void);
Oid CuckooDistributedTransactionRelationIndexId(void);

#endif