/* Copyright (c) 2025 Huawei Technologies Co., Ltd.
 * SPDX-License-Identifier: MulanPSL-2.0
 */

#ifndef CUCKOO_TRANSACTION_H
#define CUCKOO_TRANSACTION_H

#include "postgres.h"
#include "lib/stringinfo.h"

// implicit2PC_head
#define CUCKOO_TRANSACTION_2PC_HEAD "cuckoo_xact_"
#define MAX_TRANSACTION_GID_LENGTH 127
extern char PreparedTransactionGid[MAX_TRANSACTION_GID_LENGTH + 1]; //saved gid for PREPARE TRANSACTION & COMMIT PREPARED
extern char RemoteTransactionGid[MAX_TRANSACTION_GID_LENGTH + 1]; //saved gid for remote 2pc

//explicit transaction state
//use by meta_func
typedef enum CuckooExplicitTransactionState 
{
    CUCKOO_EXPLICIT_TRANSACTION_NONE,
    CUCKOO_EXPLICIT_TRANSACTION_BEGIN,
    CUCKOO_EXPLICIT_TRANSACTION_PREPARED
} CuckooExplicitTransactionState;

//remote transaction state
typedef enum CuckooRemoteTransactionState 
{
    CUCKOO_REMOTE_TRANSACTION_NONE,
    CUCKOO_REMOTE_TRANSACTION_BEGIN_FOR_WRITE,
    CUCKOO_REMOTE_TRANSACTION_BEGIN_FOR_SNAPSHOT,
    CUCKOO_REMOTE_TRANSACTION_PREPARE
} CuckooRemoteTransactionState;

// same as begin. the caller should call CuckooExplicitTransactionCommit to commit or rollback explicitly.
// however, if the transaction is aborted, we will call CuckooExpllcitTransnctionRollback
// to rollback automatically.
extern void CuckooExplicitTransactionBegin(void);
extern bool CuckooExplicitTransactionCommit(void);
extern void CuckooExplicitTransactionRollback(void);
extern bool CuckooExplicitTransactionPrepare(const char* gid);
extern void CuckooExplicitTransactionCommitPrepared(const char* gid);
extern void CuckooExplicitTransactionRollbackPrepared(const char* gid);

void RegisterCuckooTransactionCallback(void);

void TransactionManagerInit(void);

StringInfo GetImplicitTransactionGid(void);

#endif