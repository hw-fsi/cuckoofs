/* Copyright (c) 2025 Huawei Technologies Co., Ltd.
 * SPDX-License-Identifier: MulanPSL-2.0
 */

#include "metadb/meta_process_info.h"

#include "utils/utils_standalone.h"

int pg_qsort_meta_process_info_by_path_cmp(const void *a, const void *b)
{
    MetaProcessInfo pa = *(MetaProcessInfo *)a;
    MetaProcessInfo pb = *(MetaProcessInfo *)b;
    return pathcmp(pa->path, pb->path);
}