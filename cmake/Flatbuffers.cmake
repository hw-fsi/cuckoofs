find_program(FLATC flatc REQUIRED)

set(FLATBUFFERS_SCHEMA_DIR ${CMAKE_CURRENT_SOURCE_DIR}/remote_connection_def/fbs)
set(FLATBUFFERS_GENERATED_DIR ${CMAKE_BINARY_DIR}/generated)
set(SCHEMA_FILES ${FLATBUFFERS_SCHEMA_DIR}/cuckoo_meta_param.fbs ${FLATBUFFERS_SCHEMA_DIR}/cuckoo_meta_response.fbs)
set(GENERATED_FILES)
foreach(FILE ${SCHEMA_FILES})
    string(REGEX REPLACE "${FLATBUFFERS_SCHEMA_DIR}/(.*).fbs" "${FLATBUFFERS_GENERATED_DIR}/\\1_generated.h" NEW_FILE ${FILE})
    list(APPEND GENERATED_FILES ${NEW_FILE})
endforeach()

add_custom_command(
    OUTPUT ${GENERATED_FILES}
    COMMAND ${FLATC} -c -o ${FLATBUFFERS_GENERATED_DIR} ${SCHEMA_FILES}
    DEPENDS ${SCHEMA_FILES}
)

add_custom_target(GeneratedFlatBuffers ALL DEPENDS ${GENERATED_FILES})
