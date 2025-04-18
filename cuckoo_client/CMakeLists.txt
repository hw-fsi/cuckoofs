# ==================== CuckooClient =================
file(GLOB_RECURSE CUCKOO_SRC "${PROJECT_SOURCE_DIR}/cuckoo_client/src/*.cpp" "${PROJECT_SOURCE_DIR}/cuckoo_client/src/*.c")

add_library(CuckooClient
    ${CUCKOO_SRC}
    ${PROTO_SRC}
)

target_link_libraries(CuckooClient PUBLIC
    ${BRPC_LIBRARIES}
    ${DYNAMIC_LIB}
    pthread
    pq
    fuse
)
target_link_directories(CuckooClient PUBLIC ${PG_INSTALL_PATH}/lib)
target_include_directories(CuckooClient PUBLIC
    ${POSTGRES_SRC_DIR}/src/include
    ${POSTGRES_SRC_DIR}/src/interfaces/libpq
    ${PROJECT_SOURCE_DIR}/cuckoo_client/src/include
    ${PROJECT_SOURCE_DIR}/cuckoo_store/src/include
    ${PROJECT_SOURCE_DIR}/common/src/include
)
