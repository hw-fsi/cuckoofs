/* Copyright (c) 2025 Huawei Technologies Co., Ltd.
 * SPDX-License-Identifier: MulanPSL-2.0
 */

#include "metadb/inode_table.h"

const char *InodeTableName = "cuckoo_inode_table";

void ConstructCreateInodeTableCommand(StringInfo command, const char *name)
{
    appendStringInfo(command,
                     "CREATE TABLE cuckoo.%s(name			   varchar(256),"
                     "st_ino			   bigint,"
                     "parentid_partid   bigint,"
                     "st_dev			   bigint,"
                     "st_mode		   int,"
                     "st_nlink		   bigint,"
                     "st_uid			   int,"
                     "st_gid			   int,"
                     "st_rdev		   bigint,"
                     "st_size		   bigint,"
                     "st_blksize		   bigint,"
                     "st_blocks		   bigint,"
                     "st_atim		   timestamptz,"
                     "st_mtim		   timestamptz,"
                     "st_ctim		   timestamptz,"
                     "etag			   text,"
                     "update_version	   bigint,"
                     "primary_nodeid	   int,"
                     "backup_nodeid	   int);"
                     "CREATE UNIQUE INDEX %s_index ON cuckoo.%s USING btree(parentid_partid, name);"
                     "ALTER TABLE cuckoo.%s SET SCHEMA pg_catalog;"
                     "GRANT SELECT ON pg_catalog.%s TO public;"
                     "ALTER EXTENSION cuckoo ADD TABLE %s;",
                     name,
                     name,
                     name,
                     name,
                     name,
                     name);
}