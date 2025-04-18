/* Copyright (c) 2025 Huawei Technologies Co., Ltd.
 * SPDX-License-Identifier: MulanPSL-2.0
 */

/*
 * directory_table.h
 *      definition of the "cuckoo_directory_table" relation
 */

#ifndef CUCKOO_DIRECTORY_TABLE_H
#define CUCKOO_DIRECTORY_TABLE_H

#include "postgres.h"
#include "catalog/indexing.h"
#include "utils/relcache.h"

#include "metadb/metadata.h"

typedef struct FormData_cuckoo_directory_table 
{
    int64_t parentId;
    text    name;
    int64_t inodeId;
} FormData_cuckoo_directory_table;

typedef FormData_cuckoo_directory_table *Form_cuckoo_directory_table;

#define Natts_cuckoo_directory_table 3
#define Anum_cuckoo_directory_table_parent_id 1
#define Anum_cuckoo_directory_table_name 2
#define Anum_cuckoo_directory_table_inode_id 3

typedef enum CuckooDirectoryTableScankeyType 
{
    DIRECTORY_TABLE_PARENT_ID_EQ = 0,
    DIRECTORY_TABLE_NAME_EQ,
    LAST_CUCKOO_DIRECTORY_TABLE_SCANKEY_TYPE
} CuckooDirectoryTableScankeyType;

extern const char* DirectoryTableName;
Oid DirectoryRelationId(void);
Oid DirectoryRelationIndexId(void);
void SearchDirectoryTableInfo(Relation directoryRel, uint64_t parentId, const char* name, uint64_t* inodeId);
void InsertIntoDirectoryTable(Relation directoryRel, CatalogIndexState indexState, 
                            uint64_t parentId, const char* name, uint64_t inodeId);
void DeleteFromDirectoryTable(Relation directoryRel, uint64_t parentId, const char* name);

#endif