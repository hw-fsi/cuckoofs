/* Copyright (c) 2025 Huawei Technologies Co., Ltd.
 * SPDX-License-Identifier: MulanPSL-2.0
 */

#include "transaction/transaction.h"

#include "access/twophase.h"
#include "access/xact.h"
#include "catalog/pg_namespace_d.h"
#include "cuckoo_config.h"
#include "miscadmin.h"
#include "utils/elog.h"
#include "utils/guc.h"
#include "utils/lsyscache.h"
#include "utils/memutils.h"
#include "utils/snapmgr.h"

#include "control/control_flag.h"
#include "dir_path_shmem/dir_path_hash.h"
#include "distributed_backend/remote_comm.h"
#include "distributed_backend/remote_comm_cuckoo.h"
#include "metadb/foreign_server.h"
#include "transaction/transaction_cleanup.h"
#include "utils/error_log.h"
#include "utils/path_parse.h"
#include "utils/rwlock.h"
#include "utils/utils.h"

static CuckooExplicitTransactionState cuckooExplicitTransactionState = CUCKOO_EXPLICIT_TRANSACTION_NONE;

static void CuckooTransactionCallback(XactEvent event, void *args);

char PreparedTransactionGid[MAX_TRANSACTION_GID_LENGTH + 1];
char RemoteTransactionGid[MAX_TRANSACTION_GID_LENGTH + 1];

static void ClearRemoteTransactionGid()
{
    if (RemoteTransactionGid[0] != '\0') {
        RemoveInprogressTransaction(RemoteTransactionGid);
    }
    RemoteTransactionGid[0] = '\0';
}

StringInfo GetImplicitTransactionGid()
{
    StringInfo str = makeStringInfo();
    appendStringInfo(str,
                     "%s%u:%d:%c",
                     CUCKOO_TRANSACTION_2PC_HEAD,
                     GetTopTransactionId(),
                     GetLocalServerId(),
                     IsLocalWrite() ? 'T' : 'F');
    if (strlen(str->data) > MAX_TRANSACTION_GID_LENGTH)
        CUCKOO_ELOG_ERROR_EXTENDED(PROGRAM_ERROR,
                                   "length of transaction gid is expected to below %d",
                                   MAX_TRANSACTION_GID_LENGTH);
    return str;
}

void CuckooExplicitTransactionBegin()
{
    if (cuckooExplicitTransactionState != CUCKOO_EXPLICIT_TRANSACTION_NONE)
        CUCKOO_ELOG_ERROR(PROGRAM_ERROR, "incorrect transaction state.");

    BeginTransactionBlock();
    CommitTransactionCommand();
    int setIsolationLevelRes = set_config_option("transaction_isolation",
                                                 "repeatable read",
                                                 (superuser() ? PGC_SUSET : PGC_USERSET),
                                                 PGC_S_SESSION,
                                                 GUC_ACTION_SET,
                                                 true,
                                                 0,
                                                 false);
    if (setIsolationLevelRes != 1)
        CUCKOO_ELOG_ERROR(PROGRAM_ERROR, "isolation level set error.");

    cuckooExplicitTransactionState = CUCKOO_EXPLICIT_TRANSACTION_BEGIN;
}

// may cause rollback
bool CuckooExplicitTransactionCommit()
{
    if (cuckooExplicitTransactionState != CUCKOO_EXPLICIT_TRANSACTION_BEGIN)
        CUCKOO_ELOG_ERROR(PROGRAM_ERROR, "incorrect transaction state.");

    bool isCommit = EndTransactionBlock(false);
    CommitTransactionCommand();

    cuckooExplicitTransactionState = CUCKOO_EXPLICIT_TRANSACTION_NONE;
    return isCommit;
}

void CuckooExplicitTransactionRollback()
{
    if (cuckooExplicitTransactionState != CUCKOO_EXPLICIT_TRANSACTION_BEGIN)
        CUCKOO_ELOG_ERROR(PROGRAM_ERROR, "incorrect transaction state.");

    UserAbortTransactionBlock(false);
    CommitTransactionCommand();

    cuckooExplicitTransactionState = CUCKOO_EXPLICIT_TRANSACTION_NONE;
}

extern bool CuckooExplicitTransactionPrepare(const char *gid)
{
    if (cuckooExplicitTransactionState != CUCKOO_EXPLICIT_TRANSACTION_BEGIN)
        CUCKOO_ELOG_ERROR(PROGRAM_ERROR, "incorrect transaction state.");

    bool isPrepare = PrepareTransactionBlock(gid);
    CommitTransactionCommand();

    cuckooExplicitTransactionState = CUCKOO_EXPLICIT_TRANSACTION_NONE;
    return isPrepare;
}

extern void CuckooExplicitTransactionCommitPrepared(const char *gid)
{
    if (cuckooExplicitTransactionState != CUCKOO_EXPLICIT_TRANSACTION_PREPARED)
        CUCKOO_ELOG_ERROR(PROGRAM_ERROR, "incorrect transaction state.");

    PreventInTransactionBlock(true, "COMMIT PREPARED");
    FinishPreparedTransaction(gid, true);
    CommitTransactionCommand();

    cuckooExplicitTransactionState = CUCKOO_EXPLICIT_TRANSACTION_NONE;
}
extern void CuckooExplicitTransactionRollbackPrepared(const char *gid)
{
    if (cuckooExplicitTransactionState != CUCKOO_EXPLICIT_TRANSACTION_PREPARED)
        CUCKOO_ELOG_ERROR(PROGRAM_ERROR, "incorrect transaction state.");

    PreventInTransactionBlock(true, "ROLLBACK PREPARED");
    FinishPreparedTransaction(gid, false);
    CommitTransactionCommand();

    cuckooExplicitTransactionState = CUCKOO_EXPLICIT_TRANSACTION_NONE;
}

void RegisterCuckooTransactionCallback(void) { RegisterXactCallback(CuckooTransactionCallback, NULL); }

static void CuckooTransactionCallback(XactEvent event, void *args)
{
    switch (event) {
    case XACT_EVENT_PRE_COMMIT: {
        CuckooRemoteCommandPrepare();
        break;
    }
    case XACT_EVENT_COMMIT: {
        CuckooRemoteCommandCommit();

        TransactionLevelPathParseReset();
        CommitForDirPathHash();
        RWLockReleaseAll(false);
        ClearRemoteTransactionGid();
        ClearRemoteConnectionCommand();
        break;
    }
    case XACT_EVENT_ABORT: {
        CuckooEnterAbortProgress();

        TransactionLevelPathParseReset();
        AbortForDirPathHash();
        RWLockReleaseAll(true);
        if (!CuckooRemoteCommandAbort())
            elog(WARNING, "%s, Abort failed.", CuckooErrorCodeToString[PROGRAM_ERROR]);
        ClearRemoteTransactionGid();
        ClearRemoteConnectionCommand();
        if (cuckooExplicitTransactionState == CUCKOO_EXPLICIT_TRANSACTION_BEGIN)
            CuckooExplicitTransactionRollback();

        CuckooQuitAbortProgress();
        break;
    }
    case XACT_EVENT_PRE_PREPARE: {
        break;
    }
    case XACT_EVENT_PREPARE: {
        TransactionLevelPathParseReset();
        break;
    }
    case XACT_EVENT_PARALLEL_COMMIT:
    case XACT_EVENT_PARALLEL_PRE_COMMIT:
    case XACT_EVENT_PARALLEL_ABORT: {
        break;
    }
    }
}