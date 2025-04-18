/* Copyright (c) 2025 Huawei Technologies Co., Ltd.
 * SPDX-License-Identifier: MulanPSL-2.0
 */

#ifndef CUCKOO_METADB_META_HANDLE_H
#define CUCKOO_METADB_META_HANDLE_H

#include <stdbool.h>
#include "metadb/meta_process_info.h"
#include "remote_connection_utils/serialized_data.h"

#define DEFAULT_SUBPART_NUM 100

extern MemoryManager PgMemoryManager;

typedef enum CuckooSupportMetaService 
{
    PLAIN_COMMAND,
    MKDIR,
    MKDIR_SUB_MKDIR,
    MKDIR_SUB_CREATE,
    CREATE,
    STAT,
    OPEN,
    CLOSE,
    UNLINK,
    READDIR,
    OPENDIR,
    RMDIR,
    RMDIR_SUB_RMDIR,
    RMDIR_SUB_UNLINK,
    RENAME,
    RENAME_SUB_RENAME_LOCALLY,
    RENAME_SUB_CREATE,
    UTIMENS,
    CHOWN,
    CHMOD,
    NOT_SUPPORTED
} CuckooSupportMetaService;

//func whose name ends with internal is not supposed to be called by external user
void CuckooMkdirHandle(MetaProcessInfo *infoArray, int count);
void CuckooMkdirSubMkdirHandle(MetaProcessInfo *infoArray, int count);
void CuckooMkdirSubCreateHandle(MetaProcessInfo *infoArray, int count);
void CuckooCreateHandle(MetaProcessInfo *infoArray, int count, bool updateExisted);
void CuckooStatHandle(MetaProcessInfo *infoArray, int count);
void CuckooOpenHandle(MetaProcessInfo *infoArray, int count);
void CuckooCloseHandle(MetaProcessInfo *infoArray, int count);
void CuckooUnlinkHandle(MetaProcessInfo* infoArray, int count);
void CuckooReadDirHandle(MetaProcessInfo info);
void CuckooOpenDirHandle(MetaProcessInfo info);
void CuckooRmdirHandle(MetaProcessInfo info);
void CuckooRmdirSubRmdirHandle(MetaProcessInfo info);
void CuckooRmdirSubUnlinkHandle(MetaProcessInfo info);
void CuckooRenameHandle(MetaProcessInfo info);
void CuckooRenameSubRenameLocallyHandle(MetaProcessInfo info);
void CuckooRenameSubCreateHandle(MetaProcessInfo info);
void CuckooUtimeNsHandle(MetaProcessInfo info);
void CuckooChownHandle(MetaProcessInfo info);
void CuckooChmodHandle(MetaProcessInfo info);

#endif
