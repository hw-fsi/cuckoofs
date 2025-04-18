/* Copyright (c) 2025 Huawei Technologies Co., Ltd.
 * SPDX-License-Identifier: MulanPSL-2.0
 */

#ifndef CUCKOO_FOREIGN_SERVER_H
#define CUCKOO_FOREIGN_SERVER_H

#include "postgres.h"

#include "lib/stringinfo.h"
#include "libpq-fe.h"
#include "nodes/pg_list.h"
#include "utils/hsearch.h"

#include "transaction/transaction.h"

#define CUCKOO_CN_SERVER_ID 0

#define FOREIGN_SERVER_NAME_MAX_LENGTH  128
#define HOST_MAX_LENGTH                 16
#define FOREIGN_SERVER_NUM_EXPECT       8
#define FOREIGN_SERVER_NUM_MAX          128

typedef struct FormData_cuckoo_foreign_server 
{
    int32_t server_id;
    char    server_name[FOREIGN_SERVER_NAME_MAX_LENGTH];
    char    host[HOST_MAX_LENGTH];
    int32_t port;
    bool    is_local;
    char    user_name[FOREIGN_SERVER_NAME_MAX_LENGTH];
} FormData_cuckoo_foreign_server;
typedef FormData_cuckoo_foreign_server *Form_cuckoo_foreign_server;

#define Natts_cuckoo_foreign_server 6
#define Anum_cuckoo_foreign_server_server_id 1
#define Anum_cuckoo_foreign_server_server_name 2
#define Anum_cuckoo_foreign_server_host 3
#define Anum_cuckoo_foreign_server_port 4
#define Anum_cuckoo_foreign_server_is_local 5
#define Anum_cuckoo_foreign_server_user_name 6

Oid ForeignServerRelationId(void);
Oid ForeignServerRelationIndexId(void);

size_t ForeignServerShmemsize(void);
void ForeignServerShmemInit(void);

void InvalidateForeignServerShmemCache(void);
void InvalidateForeignServerShmemCacheCallback(Datum argument, Oid relationId);
void ReloadForeignServerShmemCache(void);

void ForeignServerCacheInit(void);


// c api to insert foreign server
void InsertForeignServer(const int32_t serverId, const char* serverName, const char* host, 
                        const int32_t port, const bool isLocal, const char* userName);
void DeleteForeignServer(const int32_t serverId);
void UpdateForeignServer(const int32_t serverId, const char* host, const int32_t port);

int32_t GetForeignServerCount(void);
List* GetForeignServerInfo(List* foreignServerIdList);
List* GetForeignServerConnection(List* foreignServerIdList);
List* GetForeignServerConnectionInfo(List* foreignServerIdList);
int32_t GetLocalServerId(void);
const char* GetLocalServerName(void);
List* GetAllForeignServerId(bool exceptSelf, bool exceptCn);

void CleanupForeignServerConnections(void);

typedef struct ForeignServerConnection 
{
    int32_t serverId;
    PGconn* conn;
    CuckooRemoteTransactionState transactionState;
} ForeignServerConnection;

typedef struct ForeignServerConnectionInfo 
{
    char host[HOST_MAX_LENGTH];
    int32_t port;
} ForeignServerConnectionInfo;

typedef enum CuckooForeignServerTableScankeyType 
{
    FOREIGN_SERVER_TABLE_SERVER_ID_EQ = 0,
    LAST_CUCKOO_FOREIGN_SERVER_TABLE_SCANKEY_TYPE
} CuckooForeignServerTableScankeyType;

#endif