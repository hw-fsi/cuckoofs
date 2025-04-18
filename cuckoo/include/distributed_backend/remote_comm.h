/* Copyright (c) 2025 Huawei Technologies Co., Ltd.
 * SPDX-License-Identifier: MulanPSL-2.0
 */

#ifndef CUCKOO_REMOTE_COMM_H
#define CUCKOO_REMOTE_COMM_H

#include "postgres.h"

#include "lib/stringinfo.h"
#include "nodes/pg_list.h"
#include "utils/utils.h"

#define CN_MAX_NUM 1
#define DN_MAX_NUM 64

typedef struct RemoteCommandResultPerServerData
{
    int32_t serverId;
    List *remoteCommandResult; // each item is PGresult*
} RemoteCommandResultPerServerData;
typedef List *MultipleServerRemoteCommandResult;

#endif