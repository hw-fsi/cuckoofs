find_path(LevelDB_INCLUDE_DIRS NAMES leveldb/db.h PATHS /usr/local/include /usr/include)
find_library(LevelDB_LIBRARIES NAMES leveldb PATHS /usr/local/lib /usr/lib)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(LevelDB DEFAULT_MSG LevelDB_INCLUDE_DIRS LevelDB_LIBRARIES)
