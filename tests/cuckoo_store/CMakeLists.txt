include(GoogleTest)

enable_testing()
link_directories(${POSTGRES_SRC_DIR}/src/interfaces/libpq)

# ==================== CuckooStoreUT =================
add_executable(CuckooStoreUT
    ${PROJECT_SOURCE_DIR}/tests/cuckoo_store/test_cuckoo_store.cpp
    ${common_src}
)
target_link_libraries(CuckooStoreUT
    CuckooStore
    CuckooClient
    zookeeper_mt
    glog
    jsoncpp
    gtest
    pq
    ${BRPC_LIBRARIES}
    ${DYNAMIC_LIB}
)

gtest_discover_tests(CuckooStoreUT)

# ==================== CuckooStoreThreadedUT =================
add_executable(CuckooStoreThreadedUT
    ${PROJECT_SOURCE_DIR}/tests/cuckoo_store/test_cuckoo_store_threaded.cpp
    ${common_src}
)
target_link_libraries(CuckooStoreThreadedUT
    CuckooStore
    CuckooClient
    zookeeper_mt
    glog
    jsoncpp
    gtest
    pq
    ${BRPC_LIBRARIES}
    ${DYNAMIC_LIB}
)

gtest_discover_tests(CuckooStoreThreadedUT)

# ==================== FileLockUT =================
add_executable(FileLockUT
    ${PROJECT_SOURCE_DIR}/tests/cuckoo_store/test_file_lock.cpp
)
target_link_libraries(FileLockUT
    CuckooStore
    gtest
)

gtest_discover_tests(FileLockUT)

# ==================== ReadStreamUT =================

add_executable(ReadStreamUT
    ${PROJECT_SOURCE_DIR}/tests/cuckoo_store/test_read_stream.cpp
    ${common_src}
)
target_link_libraries(ReadStreamUT
    CuckooStore
    CuckooClient
    zookeeper_mt
    glog
    jsoncpp
    gtest
    pq
    ${BRPC_LIBRARIES}
    ${DYNAMIC_LIB}
)

gtest_discover_tests(ReadStreamUT)

# ==================== NodeUT =================

add_executable(NodeUT
    ${PROJECT_SOURCE_DIR}/tests/cuckoo_store/test_node.cpp
    ${common_src}
)
target_link_libraries(NodeUT
    CuckooStore
    CuckooClient
    zookeeper_mt
    glog
    jsoncpp
    gtest
    pq
    ${BRPC_LIBRARIES}
    ${DYNAMIC_LIB}
)

gtest_discover_tests(NodeUT)

# ==================== DiskCacheUT =================

add_executable(DiskCacheUT
    ${PROJECT_SOURCE_DIR}/tests/cuckoo_store/test_disk_cache.cpp
)
target_link_libraries(DiskCacheUT
    CuckooStore
    gtest
)

gtest_discover_tests(DiskCacheUT)