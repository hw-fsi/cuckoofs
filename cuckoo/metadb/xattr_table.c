/* Copyright (c) 2025 Huawei Technologies Co., Ltd.
 * SPDX-License-Identifier: MulanPSL-2.0
 */

#include "metadb/xattr_table.h"

#include "metadb/metadata.h"

const char *XattrTableName = "cuckoo_xattr_table";

void ConstructCreateXattrTableCommand(StringInfo command, const char *name)
{
    appendStringInfo(command,
                     "CREATE TABLE cuckoo.%s(parentid_partid bigint,"
                     "name text,"
                     "xKey text,"
                     "xValue text);"
                     "CREATE UNIQUE INDEX %s_index ON cuckoo.%s USING btree(parentid_partid, name, xKey);"
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