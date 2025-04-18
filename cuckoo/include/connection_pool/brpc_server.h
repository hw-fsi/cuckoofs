/* Copyright (c) 2025 Huawei Technologies Co., Ltd.
 * SPDX-License-Identifier: MulanPSL-2.0
 */

#ifndef CUCKOO_CONNECTION_POOL_BRPC_SERVER_H
#define CUCKOO_CONNECTION_POOL_BRPC_SERVER_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

bool PG_RunConnectionPoolBrpcServer(void);
bool PG_ShutdownConnectionPoolBrpcServer(void);

#ifdef __cplusplus
}
#endif

#endif