INCLUDE(FindPackageHandleStandardArgs)

find_path(
   UV_INCLUDE_DIR
   NAMES uv uv.h
   PATHS /usr/include /usr/local/include
)

find_library(
   UV_LIBRARY
   NAMES uv libuv
   PATHS /usr/lib /usr/local/lib
)

if(WIN32)
    list(APPEND UV_LIBRARY iphlpapi)
    list(APPEND UV_LIBRARY psapi)
    list(APPEND UV_LIBRARY userenv)
    list(APPEND UV_LIBRARY ws2_32)
endif()

find_package_handle_standard_args(uv DEFAULT_MSG UV_INCLUDE_DIR UV_LIBRARY)

if(UV_FOUND)
    set(UV_INCLUDE_DIRS ${UV_INCLUDE_DIR})
    set(UV_LIBRARIES ${UV_LIBRARY})
    message(STATUS "Found libuv (include: ${UV_INCLUDE_DIRS}, libraries: ${UV_LIBRARIES})")
endif()