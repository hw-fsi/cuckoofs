/* Copyright (c) 2025 Huawei Technologies Co., Ltd.
 * SPDX-License-Identifier: MulanPSL-2.0
 */

#ifndef CUCKOO_DISTRIBUTED_BACKEND_CUCKOO_H
#define CUCKOO_DISTRIBUTED_BACKEND_CUCKOO_H

#include "postgres.h"

#include "metadb/metadata.h"

void CuckooCreateDistributedDataTable(void);
void CuckooPrepareCommands(void);

#endif