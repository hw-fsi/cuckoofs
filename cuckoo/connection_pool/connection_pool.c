/* Copyright (c) 2025 Huawei Technologies Co., Ltd.
 * SPDX-License-Identifier: MulanPSL-2.0
 */

#include "connection_pool/connection_pool.h"

#include <signal.h>
#include <threads.h>
#include <unistd.h>

#include "postgres.h"

#include "postmaster/bgworker.h"
#include "postmaster/postmaster.h"
#include "storage/shmem.h"
#include "utils/error_log.h"
#include "utils/memutils.h"
#include "utils/resowner.h"

#include "connection_pool/brpc_server.h"
#include "control/control_flag.h"

int CuckooPGPort = 0;
int CuckooConnectionPoolPort = CUCKOO_CONNECTION_POOL_PORT_DEFAULT;
int CuckooConnectionPoolSize = CUCKOO_CONNECTION_POOL_SIZE_DEFAULT;
uint64_t CuckooConnectionPoolShmemSize = CUCKOO_CONNECTION_POOL_SHMEM_SIZE_DEFAULT;
static char *CuckooConnectionPoolShmemBuffer = NULL;
CuckooShmemAllocator CuckooConnectionPoolShmemAllocator;

static volatile bool got_SIGTERM = false;
static void CuckooDaemonConnectionPoolProcessSigTermHandler(SIGNAL_ARGS);

void CuckooDaemonConnectionPoolProcessMain(unsigned long int main_arg)
{
    pqsignal(SIGTERM, CuckooDaemonConnectionPoolProcessSigTermHandler);
    BackgroundWorkerUnblockSignals();
    BackgroundWorkerInitializeConnection("postgres", NULL, 0);

    CurrentResourceOwner = ResourceOwnerCreate(NULL, "cuckoo connection pool");
    CurrentMemoryContext = AllocSetContextCreate(TopMemoryContext,
                                                 "cuckoo connection pool context",
                                                 ALLOCSET_DEFAULT_MINSIZE,
                                                 ALLOCSET_DEFAULT_INITSIZE,
                                                 ALLOCSET_DEFAULT_MAXSIZE);
    elog(LOG, "CuckooDaemonConnectionPoolProcessMain: pid = %d", getpid());
    elog(LOG, "CuckooDaemonConnectionPoolProcessMain: wait init.");
    bool serviceStarted = false;
    do {
        sleep(1);
        serviceStarted = CheckCuckooBackgroundServiceStarted();
    } while (!serviceStarted);
    elog(LOG, "CuckooDaemonConnectionPoolProcessMain: init finished.");

    CuckooPGPort = PostPortNumber;
    PG_RunConnectionPoolBrpcServer();

    elog(LOG, "CuckooDaemonConnectionPoolProcessMain: connection pool server stopped.");
    return;
}

static void CuckooDaemonConnectionPoolProcessSigTermHandler(SIGNAL_ARGS)
{
    int save_errno = errno;

    elog(LOG, "CuckooDaemonConnectionPoolProcessSigTermHandler: get sigterm.");
    got_SIGTERM = true;
    PG_ShutdownConnectionPoolBrpcServer();

    errno = save_errno;
}

int CuckooConnectionPoolGotSigTerm(void) { return got_SIGTERM; }

size_t CuckooConnectionPoolShmemsize() { return CuckooConnectionPoolShmemSize; }
void CuckooConnectionPoolShmemInit()
{
    bool initialized;

    CuckooConnectionPoolShmemBuffer =
        ShmemInitStruct("Cuckoo Connection Pool Shmem", CuckooConnectionPoolShmemsize(), &initialized);
    if (CuckooShmemAllocatorInit(&CuckooConnectionPoolShmemAllocator,
                                 CuckooConnectionPoolShmemBuffer,
                                 CuckooConnectionPoolShmemsize()) != 0)
        CUCKOO_ELOG_ERROR(PROGRAM_ERROR, "CuckooShmemAllocatorInit failed.");
    if (!initialized) {
        memset(CuckooConnectionPoolShmemAllocator.signatureCounter,
               0,
               sizeof(PaddedAtomic64) *
                   (1 + CUCKOO_SHMEM_ALLOCATOR_FREE_LIST_COUNT + CuckooConnectionPoolShmemAllocator.pageCount));
    }
}