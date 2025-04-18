/* Copyright (c) 2025 Huawei Technologies Co., Ltd.
 * SPDX-License-Identifier: MulanPSL-2.0
 */

/*
 *
 * inode_table.h
 *      definition of the "dfs_inode_table" relation
 * 
 * 
 * 
 * 
 */

#ifndef DFS_INODE_TABLE_H
#define DFS_INODE_TABLE_H

#include "metadb/metadata.h"

#define PART_ID_MASK        0x1FFF
#define PART_ID_BIT_COUNT   13

typedef struct FormData_dfs_inode_table 
{
    char name[FILENAMELENGTH];
    uint64 st_ino;
    uint64 parentid_partid;
    uint64 st_dev;
    uint32 st_mode;
    uint64 st_nlink;
    uint32 st_uid;
    uint32 st_gid;
    uint64 st_rdev;
    int64 st_size;
    int64 st_blksize;
    int64 st_blocks;
    TimestampTz st_atim;
    TimestampTz st_mtim;
    TimestampTz st_ctim;
    char etag[128];
    uint64 update_version;
    int32 primary_nodeid;
    int32 backup_nodeid;
} FormData_dfs_inode_table;

typedef FormData_dfs_inode_table *Form_dfs_inode_table;

#define Natts_pg_dfs_inode_table 19
#define Anum_pg_dfs_file_name 1
#define Anum_pg_dfs_file_st_ino 2
#define Anum_pg_dfs_file_parentid_partid 3
#define Anum_pg_dfs_file_st_dev 4
#define Anum_pg_dfs_file_st_mode 5
#define Anum_pg_dfs_file_st_nlink 6
#define Anum_pg_dfs_file_st_uid 7
#define Anum_pg_dfs_file_st_gid 8
#define Anum_pg_dfs_file_st_rdev 9
#define Anum_pg_dfs_file_st_size 10
#define Anum_pg_dfs_file_st_blksize 11
#define Anum_pg_dfs_file_st_blocks 12
#define Anum_pg_dfs_file_st_atim 13
#define Anum_pg_dfs_file_st_mtim 14
#define Anum_pg_dfs_file_st_ctim 15
#define Anum_pg_dfs_file_etag 16
#define Anum_pg_dfs_file_update_version 17
#define Anum_pg_dfs_file_primary_nodeid 18
#define Anum_pg_dfs_file_backup_nodeid 19

// #define DFS_INODEID_SEQUENCE_NAME "pg_dfs_inodeid_seq"

typedef enum CuckooInodeTableScankeyType 
{
    INODE_TABLE_PARENT_ID_PART_ID_GT = 0,
    INODE_TABLE_PARENT_ID_PART_ID_GE,
    INODE_TABLE_PARENT_ID_PART_ID_LE,
    INODE_TABLE_PARENT_ID_PART_ID_EQ,
    INODE_TABLE_NAME_GT,
    INODE_TABLE_NAME_EQ,
    LAST_CUCKOO_INODE_TABLE_SCANKEY_TYPE
} CuckooInodeTableScankeyType;

typedef enum CuckooInodeTableIndexParentIdPartIdNameScankeyType 
{
    INODE_TABLE_INDEX_PARENT_ID_PART_ID_EQ = 0,
    INODE_TABLE_INDEX_NAME_EQ,
    LAST_CUCKOO_INODE_TABLE_INDEX_PARENT_ID_PART_ID_NAME_SCANKEY_TYPE
} CuckooInodeTableIndexParentIdPartIdNameScankeyType;

typedef enum StreamSearchState 
{
    SAME_ID_GREATER_NAME = 0,
    GREATER_ID,
    NEW_SHARD
} StreamSearchState;

extern const char* InodeTableName;
void ConstructCreateInodeTableCommand(StringInfo command, const char* name);

#endif