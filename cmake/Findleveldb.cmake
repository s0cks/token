include(FindPackageHandleStandardArgs)

find_path(
   LEVELDB_INCLUDE_DIR
   NAMES leveldb/db.h
   PATHS /usr/include;/usr/local/include
)
find_library(
   LEVELDB_LIBRARY
   NAMES leveldb
   PATHS /usr/lib;/usr/local/lib
)

find_package_handle_standard_args(leveldb DEFAULT_MSG LEVELDB_INCLUDE_DIR LEVELDB_LIBRARY)
if(LEVELDB_FOUND)
    set(LEVELDB_INCLUDE_DIRS ${LEVELDB_INCLUDE_DIR})
    set(LEVELDB_LIBRARIES ${LEVELDB_LIBRARY})
    message(STATUS "Found leveldb (include: ${LEVELDB_INCLUDE_DIRS}, libraries: ${LEVELDB_LIBRARIES})")
endif()