include(FindPackageHandleStandardArgs)

find_path(
   GLOG_INCLUDE_DIR
   NAMES glog/logging.h
   PATHS /usr/include;/usr/local/include
)
find_library(
   GLOG_LIBRARY
   NAMES glog
   PATHS /usr/lib;/usr/local/lib
)

find_package_handle_standard_args(glog DEFAULT_MSG GLOG_INCLUDE_DIR GLOG_LIBRARY)

if(GLOG_FOUND)
    set(GLOG_INCLUDE_DIRS ${GLOG_INCLUDE_DIR})
    set(GLOG_LIBRARIES ${GLOG_LIBRARY})
    message(STATUS "Found glog (include: ${GLOG_INCLUDE_DIRS}, libraries: ${GLOG_LIBRARIES})")
endif()