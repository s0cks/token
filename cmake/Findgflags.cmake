include(FindPackageHandleStandardArgs)

find_path(
   GFLAGS_INCLUDE_DIR
   NAMES gflags/gflags.h
   PATHS /usr/include;/usr/local/include
)
find_library(
   GFLAGS_LIBRARY
   NAMES gflags
   PATHS /usr/lib;/usr/local/lib
)

find_package_handle_standard_args(gflags DEFAULT_MSG GFLAGS_INCLUDE_DIR GFLAGS_LIBRARY)

if(GFLAGS_FOUND)
    set(GFLAGS_INCLUDE_DIRS ${GFLAGS_INCLUDE_DIR})
    set(GFLAGS_LIBRARIES ${GFLAGS_LIBRARY})
    message(STATUS "Found gflags  (include: ${GFLAGS_INCLUDE_DIRS}, libraries: ${GFLAGS_LIBRARIES})")
endif()