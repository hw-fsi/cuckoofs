/* Copyright (c) 2025 Huawei Technologies Co., Ltd.
 * SPDX-License-Identifier: MulanPSL-2.0
 */

#include "postgres.h"

#include "executor/spi.h"
#include "fmgr.h"
#include "nodes/pg_list.h"

#include "connection_pool/brpc_server.h"
#include "dir_path_shmem/dir_path_hash.h"
#include "metadb/meta_process_info.h"
#include "metadb/shard_table.h"
#include "utils/error_log.h"
#include "utils/utils.h"

PG_FUNCTION_INFO_V1(cuckoo_clear_user_data_func);
PG_FUNCTION_INFO_V1(cuckoo_clear_all_data_func);
PG_FUNCTION_INFO_V1(cuckoo_clear_cached_relation_oid_func);
PG_FUNCTION_INFO_V1(cuckoo_run_pooler_server_func);

Datum cuckoo_clear_user_data_func(PG_FUNCTION_ARGS)
{
    int spiConnectionResult = SPI_connect();
    if (spiConnectionResult != SPI_OK_CONNECT) {
        SPI_finish();
        CUCKOO_ELOG_ERROR(PROGRAM_ERROR, "could not connect to SPI manager.");
    }

    StringInfo command = makeStringInfo();
    List *shardTableData = GetShardTableData();
    for (int i = 0; i < list_length(shardTableData); ++i) {
        FormData_cuckoo_shard_table *data = list_nth(shardTableData, i);
        if (data->server_id != GetLocalServerId())
            continue;

        appendStringInfo(command, "TRUNCATE TABLE cuckoo_inode_table_" INT32_PRINT_SYMBOL ";", data->range_point);
        appendStringInfo(command, "TRUNCATE TABLE cuckoo_xattr_table_" INT32_PRINT_SYMBOL ";", data->range_point);
    }
    appendStringInfo(command, "TRUNCATE TABLE cuckoo_directory_table;");

    int spiQueryResult = SPI_execute(command->data, false, 0);
    if (spiQueryResult != SPI_OK_UTILITY) {
        SPI_finish();
        CUCKOO_ELOG_ERROR(PROGRAM_ERROR, "spi exec failed.");
    }
    ClearDirPathHash();

    SPI_finish();
    PG_RETURN_INT16(0);
}

Datum cuckoo_clear_all_data_func(PG_FUNCTION_ARGS)
{
    int spiConnectionResult = SPI_connect();
    if (spiConnectionResult != SPI_OK_CONNECT) {
        SPI_finish();
        CUCKOO_ELOG_ERROR(PROGRAM_ERROR, "could not connect to SPI manager.");
    }

    int spiQueryResult = SPI_execute("SELECT tablename FROM pg_tables WHERE tablename LIKE 'cuckoo%';", true, 0);
    if (spiQueryResult != SPI_OK_SELECT) {
        SPI_finish();
        CUCKOO_ELOG_ERROR(PROGRAM_ERROR, "spi exec failed.");
    }

    List *cuckooTableName = NIL;
    for (int i = 0; i < SPI_processed; ++i) {
        bool isNull;
        char *name = DatumGetCString(SPI_getbinval(SPI_tuptable->vals[i], SPI_tuptable->tupdesc, 1, &isNull));
        cuckooTableName = lappend(cuckooTableName, name);
    }

    StringInfo command = makeStringInfo();
    for (int i = 0; i < list_length(cuckooTableName); ++i) {
        char *name = (char *)list_nth(cuckooTableName, i);

        if (strstr(name, "cuckoo_inode_table_") != NULL || strstr(name, "cuckoo_xattr_table_") != NULL) {
            appendStringInfo(command, "ALTER EXTENSION cuckoo DROP TABLE %s; DROP TABLE %s;", name, name);
        } else {
            appendStringInfo(command, "TRUNCATE TABLE %s;", name);
        }
    }
    appendStringInfo(command, "ALTER SEQUENCE pg_dfs_inodeid_seq RESTART;");

    spiQueryResult = SPI_execute(command->data, false, 0);
    if (spiQueryResult != SPI_OK_UTILITY) {
        SPI_finish();
        CUCKOO_ELOG_ERROR(PROGRAM_ERROR, "spi exec failed.");
    }
    InvalidateForeignServerShmemCache();
    InvalidateShardTableShmemCache();
    ClearDirPathHash();

    SPI_finish();
    PG_RETURN_INT16(0);
}

Datum cuckoo_clear_cached_relation_oid_func(PG_FUNCTION_ARGS)
{
    memset(CachedRelationOid, 0, sizeof(CachedRelationOid));

    PG_RETURN_INT16(0);
}

Datum cuckoo_run_pooler_server_func(PG_FUNCTION_ARGS)
{
    PG_RunConnectionPoolBrpcServer();
    PG_RETURN_INT16(0);
}
