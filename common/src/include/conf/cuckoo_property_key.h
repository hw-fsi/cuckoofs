/* Copyright (c) 2025 Huawei Technologies Co., Ltd.
 * SPDX-License-Identifier: MulanPSL-2.0
 */

#pragma once

#include "property_key.h"

class CuckooPropertyKey : public PropertyKey {
  public:
    inline static const auto CUCKOO_LOG_DIR =
        PropertyKey::Builder("main", "cuckoo_log_dir", CUCKOO, CUCKOO_STRING).build();

    inline static const auto CUCKOO_LOG_LEVEL =
        PropertyKey::Builder("main", "cuckoo_log_level", CUCKOO, CUCKOO_STRING).build();

    inline static const auto CUCKOO_LOG_MAX_SIZE_MB =
        PropertyKey::Builder("main", "cuckoo_log_max_size_mb", CUCKOO, CUCKOO_UINT).build();

    inline static const auto CUCKOO_THREAD_NUM =
        PropertyKey::Builder("main", "cuckoo_thread_num", CUCKOO, CUCKOO_UINT).build();

    inline static const auto CUCKOO_NODE_ID =
        PropertyKey::Builder("main", "cuckoo_node_id", CUCKOO, CUCKOO_UINT).build();

    inline static const auto CUCKOO_CACHE_ROOT =
        PropertyKey::Builder("main", "cuckoo_cache_root", CUCKOO, CUCKOO_STRING).build();

    inline static const auto CUCKOO_DIR_NUM =
        PropertyKey::Builder("main", "cuckoo_dir_num", CUCKOO, CUCKOO_UINT).build();

    inline static const auto CUCKOO_BLOCK_SIZE =
        PropertyKey::Builder("main", "cuckoo_block_size", CUCKOO, CUCKOO_UINT).build();

    inline static const auto CUCKOO_BIG_FILE_READ_SIZE =
        PropertyKey::Builder("main", "cuckoo_read_big_file_size", CUCKOO, CUCKOO_UINT).build();

    inline static const auto CUCKOO_CLUSTER_VIEW =
        PropertyKey::Builder("main", "cuckoo_cluster_view", CUCKOO, CUCKOO_ARRAY).build();

    inline static const auto CUCKOO_SERVER_IP =
        PropertyKey::Builder("main", "cuckoo_server_ip", CUCKOO, CUCKOO_STRING).build();

    inline static const auto CUCKOO_SERVER_PORT =
        PropertyKey::Builder("main", "cuckoo_server_port", CUCKOO, CUCKOO_STRING).build();

    inline static const auto CUCKOO_ASYNC = PropertyKey::Builder("main", "cuckoo_async", CUCKOO, CUCKOO_BOOL).build();

    inline static const auto CUCKOO_PERSIST =
        PropertyKey::Builder("main", "cuckoo_persist", CUCKOO, CUCKOO_BOOL).build();

    inline static const auto CUCKOO_PRE_BLOCKNUM =
        PropertyKey::Builder("main", "cuckoo_preblock_num", CUCKOO, CUCKOO_UINT).build();

    inline static const auto CUCKOO_EVICTION =
        PropertyKey::Builder("main", "cuckoo_eviction", CUCKOO, CUCKOO_DOUBLE).build();

    inline static const auto CUCKOO_STAT = PropertyKey::Builder("main", "cuckoo_stat", CUCKOO, CUCKOO_BOOL).build();

    inline static const auto CUCKOO_IS_INFERENCE =
        PropertyKey::Builder("main", "cuckoo_is_inference", CUCKOO, CUCKOO_BOOL).build();

    inline static const auto CUCKOO_MOUNT_PATH =
        PropertyKey::Builder("main", "cuckoo_mount_path", CUCKOO, CUCKOO_STRING).build();

    inline static const auto CUCKOO_TO_LOCAL =
        PropertyKey::Builder("main", "cuckoo_to_local", CUCKOO, CUCKOO_BOOL).build();

    inline static const auto CUCKOO_LOG_RESERVED_NUM =
        PropertyKey::Builder("main", "cuckoo_log_reserved_num", CUCKOO, CUCKOO_UINT).build();

    inline static const auto CUCKOO_LOG_RESERVED_TIME =
        PropertyKey::Builder("main", "cuckoo_log_reserved_time", CUCKOO, CUCKOO_UINT).build();
};
