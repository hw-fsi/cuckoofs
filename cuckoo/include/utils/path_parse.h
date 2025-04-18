/* Copyright (c) 2025 Huawei Technologies Co., Ltd.
 * SPDX-License-Identifier: MulanPSL-2.0
 */

#ifndef CUCKOO_UTILS_PATH_PARSE_H
#define CUCKOO_UTILS_PATH_PARSE_H

#include "postgres.h"
#include "lib/rbtree.h"
#include "utils/relcache.h"

#include "utils/error_log.h"

#define PATH_PARSE_FLAG_NOT_ROOT                                    1
#define PATH_PARSE_FLAG_TARGET_IS_DIRECTORY                         2
#define PATH_PARSE_FLAG_TARGET_TO_BE_CREATED                        4
#define PATH_PARSE_FLAG_INODE_ID_FOR_INPUT                          8
#define PATH_PARSE_FLAG_TARGET_TO_BE_DELETED                        16
#define PATH_PARSE_FLAG_ALLOW_OPERATION_UNDER_CREATED_DIRECTORY     32
#define PATH_PARSE_FLAG_ACQUIRE_SHARED_LOCK_IF_TARGET_IS_DIRECTORY  64



// PP_SHARED conflict with PP_EXCLUSIVE
// PP_EXCLUSIVE conflict with all kind of lock
// PP_EXCLUSIVE_FOR_CREATE conflict with PP_EXCLUSIVE/PP_EXCLUSIVE_FOR_CREATE
typedef enum PPLockMode 
{
    PP_NONE,
    PP_SHARED,
    PP_EXCLUSIVE,
    PP_EXCLUSIVE_FOR_CREATE,
    PP_LAST_LOCKMODE
} PPLockMode;

typedef struct PathParseRBTreeNode 
{
    RBTNode rbtnode;
    char* name;
    uint64_t inodeId;
    PPLockMode lockAcquired;
    RBTree* children;
} PathParseRBTreeNode;

typedef PathParseRBTreeNode* PathParseTree;

void PathParseMemoryContextInit(void);
void TransactionLevelPathParseReset(void);
uint64_t CheckWhetherPathExistsInDirectoryTable(Relation directoryRel, const char *path);
void PathParseTreeInit(PathParseRBTreeNode *root);
CuckooErrorCode PathParseTreeInsert(PathParseTree root, Relation directoryRel, const char* path, 
                        uint16_t flag, uint64_t *parentId, char **fileName, uint64_t *inodeId);

#define VERIFY_PATH_VALIDITY_REQUIREMENT_MUST_BE_DIRECTORY  1
#define VERIFY_PATH_VALIDITY_REQUIREMENT_MUST_BE_FILE       2

#define VERIFY_PATH_VALIDITY_PROPERTY_CAN_BE_DIRECTORY      1
#define VERIFY_PATH_VALIDITY_PROPERTY_CAN_BE_FILE           2

CuckooErrorCode VerifyPathValidity(const char *path, int32_t requirement, int32_t *property);

#define INODEID_SEQUENCE_NAME  "pg_dfs_inodeid_seq"
Oid GetSequenceRelationId(void);
uint64_t GetNextSequenceNum(void);

#endif
