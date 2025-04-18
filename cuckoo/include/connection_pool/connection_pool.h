/* Copyright (c) 2025 Huawei Technologies Co., Ltd.
 * SPDX-License-Identifier: MulanPSL-2.0
 */

#ifndef CUCKOO_CONNECTION_POOL_CONNECTION_POOL_H
#define CUCKOO_CONNECTION_POOL_CONNECTION_POOL_H

#include "connection_pool/connection_pool_config.h"
#include "utils/cuckoo_shmem_allocator.h"

void CuckooDaemonConnectionPoolProcessMain(unsigned long int main_arg);

extern CuckooShmemAllocator CuckooConnectionPoolShmemAllocator;

size_t CuckooConnectionPoolShmemsize(void);
void CuckooConnectionPoolShmemInit(void);

#endif