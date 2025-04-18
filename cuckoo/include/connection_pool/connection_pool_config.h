/* Copyright (c) 2025 Huawei Technologies Co., Ltd.
 * SPDX-License-Identifier: MulanPSL-2.0
 */

#ifndef CUCKOO_CONNECTION_POOL_CONNECTION_POOL_CONFIG_H
#define CUCKOO_CONNECTION_POOL_CONNECTION_POOL_CONFIG_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

extern int CuckooPGPort;

#define CUCKOO_CONNECTION_POOL_PORT_DEFAULT 56999
extern int CuckooConnectionPoolPort;

#define CUCKOO_CONNECTION_POOL_SIZE_DEFAULT 32
extern int CuckooConnectionPoolSize;

#define CUCKOO_CONNECTION_POOL_SHMEM_SIZE_DEFAULT (256 * 1024 * 1024)
extern uint64_t CuckooConnectionPoolShmemSize;

#define CUCKOO_CONNECTION_POOL_MAX_CONCURRENT_SOCKET 4096

int CuckooConnectionPoolGotSigTerm(void);

#ifdef __cplusplus
}
#endif

#endif