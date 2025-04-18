/* Copyright (c) 2025 Huawei Technologies Co., Ltd.
 * SPDX-License-Identifier: MulanPSL-2.0
 */

#include "postgres.h"
#include "fmgr.h"
#include "miscadmin.h"
#include "postmaster/bgworker.h"
#include "storage/ipc.h"
#include "storage/lwlock.h"
#include "storage/lwlocknames.h"
#include "storage/shmem.h"
#include "tcop/utility.h"

#include "connection_pool/connection_pool.h"
#include "control/control_flag.h"
#include "control/hook.h"
#include "dir_path_shmem/dir_path_hash.h"
#include "metadb/foreign_server.h"
#include "metadb/metadata.h"
#include "metadb/shard_table.h"
#include "transaction/transaction.h"
#include "transaction/transaction_cleanup.h"
#include "utils/guc.h"
#include "utils/path_parse.h"
#include "utils/rwlock.h"
#include "utils/shmem_control.h"

PG_MODULE_MAGIC;

void _PG_init(void);
static void CuckooStart2PCCleanupWorker(void);
static void CuckooStartConnectionPoolWorker(void);
static void InitializeCuckooShmemStruct(void);
static void RegisterCuckooConfigVariables(void);

void _PG_init(void)
{
    RegisterCuckooConfigVariables();
    InitializeCuckooShmemStruct();
    RegisterCuckooTransactionCallback();
    ForeignServerCacheInit();
    PathParseMemoryContextInit();

    pre_ExecutorStart_hook = ExecutorStart_hook;
    ExecutorStart_hook = cuckoo_ExecutorStart;
    pre_ProcessUtility_hook = ProcessUtility_hook;
    ProcessUtility_hook = cuckoo_ProcessUtility;
    pre_object_access_hook = object_access_hook;
    object_access_hook = cuckoo_object_access;

    CuckooStart2PCCleanupWorker();
    CuckooStartConnectionPoolWorker();
}

/*
 * Start 2PC cleanup process.
 */
static void CuckooStart2PCCleanupWorker(void)
{
    BackgroundWorker worker;
    BackgroundWorkerHandle *handle;
    BgwHandleStatus status;
    pid_t pid;

    MemSet(&worker, 0, sizeof(BackgroundWorker));
    strcpy(worker.bgw_name, "cuckoo_2pc_cleanup_process");
    strcpy(worker.bgw_type, "cuckoo_daemon_2pc_cleanup_process");
    worker.bgw_flags = BGWORKER_SHMEM_ACCESS | BGWORKER_BACKEND_DATABASE_CONNECTION;
    worker.bgw_start_time = BgWorkerStart_RecoveryFinished;
    worker.bgw_restart_time = 1;
    strcpy(worker.bgw_library_name, "cuckoo");
    strcpy(worker.bgw_function_name, "CuckooDaemon2PCFailureCleanupProcessMain");

    if (process_shared_preload_libraries_in_progress) {
        RegisterBackgroundWorker(&worker);
        return;
    }

    /* must set notify PID to wait for startup */
    worker.bgw_notify_pid = MyProcPid;

    if (!RegisterDynamicBackgroundWorker(&worker, &handle))
        ereport(ERROR,
                (errcode(ERRCODE_INSUFFICIENT_RESOURCES),
                 errmsg("could not register cuckoo background process"),
                 errhint("You may need to increase max_worker_processes.")));

    status = WaitForBackgroundWorkerStartup(handle, &pid);
    if (status != BGWH_STARTED)
        ereport(ERROR,
                (errcode(ERRCODE_INSUFFICIENT_RESOURCES),
                 errmsg("could not start cuckoo background process"),
                 errhint("More detials may be available in the server log.")));
}

/*
 * Start cuckoo connection pool process.
 */
static void CuckooStartConnectionPoolWorker(void)
{
    BackgroundWorker worker;
    BackgroundWorkerHandle *handle;
    BgwHandleStatus status;
    pid_t pid;

    MemSet(&worker, 0, sizeof(BackgroundWorker));
    strcpy(worker.bgw_name, "cuckoo_connection_pool_process");
    strcpy(worker.bgw_type, "cuckoo_daemon_connection_pool_process");
    worker.bgw_flags = BGWORKER_SHMEM_ACCESS | BGWORKER_BACKEND_DATABASE_CONNECTION;
    worker.bgw_start_time = BgWorkerStart_RecoveryFinished;
    worker.bgw_restart_time = 1;
    strcpy(worker.bgw_library_name, "cuckoo");
    strcpy(worker.bgw_function_name, "CuckooDaemonConnectionPoolProcessMain");

    if (process_shared_preload_libraries_in_progress) {
        RegisterBackgroundWorker(&worker);
        return;
    }

    /* must set notify PID to wait for startup */
    worker.bgw_notify_pid = MyProcPid;

    if (!RegisterDynamicBackgroundWorker(&worker, &handle))
        ereport(ERROR,
                (errcode(ERRCODE_INSUFFICIENT_RESOURCES),
                 errmsg("could not register cuckoo background process"),
                 errhint("You may need to increase max_worker_processes.")));

    status = WaitForBackgroundWorkerStartup(handle, &pid);
    if (status != BGWH_STARTED)
        ereport(ERROR,
                (errcode(ERRCODE_INSUFFICIENT_RESOURCES),
                 errmsg("could not start cuckoo background process"),
                 errhint("More detials may be available in the server log.")));
}

static shmem_request_hook_type prev_shmem_request_hook = NULL;
static shmem_startup_hook_type prev_shmem_startup_hook = NULL;
static void CuckooShmemRequest(void);
static void CuckooShmemInit(void);
static void CuckooShmemRequest(void)
{
    if (prev_shmem_request_hook)
        prev_shmem_request_hook();

    RequestAddinShmemSpace(CuckooControlShmemsize());
    RequestAddinShmemSpace(TransactionCleanupShmemsize());
    RequestAddinShmemSpace(ForeignServerShmemsize());
    RequestAddinShmemSpace(ShardTableShmemsize());
    RequestAddinShmemSpace(DirPathShmemsize());
    RequestAddinShmemSpace(CuckooConnectionPoolShmemsize());
}
static void CuckooShmemInit(void)
{
    if (prev_shmem_startup_hook != NULL) {
        prev_shmem_startup_hook();
    }

    LWLockAcquire(AddinShmemInitLock, LW_EXCLUSIVE);

    CuckooControlShmemInit();
    TransactionCleanupShmemInit();
    ForeignServerShmemInit();
    ShardTableShmemInit();
    DirPathShmemInit();
    CuckooConnectionPoolShmemInit();

    LWLockRelease(AddinShmemInitLock);
}
static void InitializeCuckooShmemStruct(void)
{
    prev_shmem_request_hook = shmem_request_hook;
    shmem_request_hook = CuckooShmemRequest;

    prev_shmem_startup_hook = shmem_startup_hook;
    shmem_startup_hook = CuckooShmemInit;
}

/* Register Cuckoo configuration variables. */
static void RegisterCuckooConfigVariables(void)
{
    DefineCustomIntVariable("cuckoo_connection_pool.port",
                            gettext_noop("Port of the pool manager."),
                            NULL,
                            &CuckooConnectionPoolPort,
                            CUCKOO_CONNECTION_POOL_PORT_DEFAULT,
                            1,
                            65535,
                            PGC_POSTMASTER,
                            0,
                            NULL,
                            NULL,
                            NULL);

    DefineCustomIntVariable("cuckoo_connection_pool.pool_size",
                            gettext_noop("Pool size of the pool manager."),
                            NULL,
                            &CuckooConnectionPoolSize,
                            CUCKOO_CONNECTION_POOL_SIZE_DEFAULT,
                            1,
                            256,
                            PGC_POSTMASTER,
                            0,
                            NULL,
                            NULL,
                            NULL);

    int CuckooConnectionPoolShmemSizeInMB = CUCKOO_CONNECTION_POOL_SHMEM_SIZE_DEFAULT / 1024 / 1024;
    DefineCustomIntVariable(gettext_noop("cuckoo_connection_pool.shmem_size"),
                            "Shmem size of the pool manager, unit: MB.",
                            NULL,
                            &CuckooConnectionPoolShmemSizeInMB,
                            CUCKOO_CONNECTION_POOL_SHMEM_SIZE_DEFAULT / 1024 / 1024,
                            32,
                            64 * 1024,
                            PGC_POSTMASTER,
                            0,
                            NULL,
                            NULL,
                            NULL);
    CuckooConnectionPoolShmemSize = (uint64_t)CuckooConnectionPoolShmemSizeInMB * 1024 * 1024;
}