
include(FindPackageHandleStandardArgs)
find_path(
   UUID_INCLUDE_DIR
   NAMES uuid
   PATHS /usr/include /usr/local/include
)
find_library(
   UUID_LIBRARY
   NAMES uuid libuuid
   PATHS /usr/lib /usr/local/lib
)

find_package_handle_standard_args(uuid DEFAULT_MSG UUID_INCLUDE_DIR UUID_LIBRARY)
if(UUID_FOUND)
    set(UUID_INCLUDE_DIRS ${UUID_INCLUDE_DIR})
    set(UUID_LIBRARIES ${UUID_LIBRARY})
    message(STATUS "Found uuid (include: ${UUID_INCLUDE_DIRS}, libraries: ${UUID_LIBRARIES})")
endif()