/* Copyright (c) 2025 Huawei Technologies Co., Ltd.
 * SPDX-License-Identifier: MulanPSL-2.0
 */

/*
 *
 * directory_path.h
 *      definition of the "dfs_directory_path" relation
 * 
 * 
 * 
 * 
 */

#ifndef DFS_DIRECTORY_PATH_H
#define DFS_DIRECTORY_PATH_H

typedef struct FormData_dfs_directory_path
{
    text name;
    uint64 inodeid;
    uint64 parentid;
    uint subpartnum;
} FormData_dfs_directory_path;

typedef FormData_dfs_directory_path *Form_dfs_directory_path;

/*
 *  Compiler constants for dfs_directory_path
 *   
 */
#define Natts_pg_dfs_path_directory 4
#define Anum_pg_dfs_name 1
#define Anum_pg_dfs_inodeid 2
#define Anum_pg_dfs_parentid 3
#define Anum_pg_dfs_subpartnum 4

#define DFS_INODEID_SEQUENCE_NAME "pg_dfs_inodeid_seq"

#endif