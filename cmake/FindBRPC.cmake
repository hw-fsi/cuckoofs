# 查找 BRPC 的头文件和库文件

find_path(BRPC_INCLUDE_DIRS NAMES brpc/server.h PATHS /usr/local/include)
find_library(BRPC_LIBRARIES NAMES brpc PATHS /usr/local/lib)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(BRPC DEFAULT_MSG BRPC_INCLUDE_DIRS BRPC_LIBRARIES)
