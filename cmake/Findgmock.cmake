include(FindPackageHandleStandardArgs)
find_path(
   GMOCK_INCLUDE_DIR
   NAMES gmock/gmock.h
   PATHS /usr/include /usr/local/include
)

find_library(
   GMOCK_LIBRARY
   NAMES gmock
   PATHS /usr/lib /usr/local/lib
)
find_library(
   GMOCK_MAIN_LIBRARY
   NAMES gmock_main
   PATHS /usr/lib /usr/local/lib
)

find_package_handle_standard_args(gmock DEFAULT_MSG GMOCK_INCLUDE_DIR GMOCK_LIBRARY)
if(GMOCK_FOUND)
  set(GMOCK_INCLUDE_DIRS ${GMOCK_INCLUDE_DIR})
  set(GMOCK_LIBRARIES ${GMOCK_LIBRARY} ${GMOCK_MAIN_LIBRARY})
  message(STATUS "Found gmock (include: ${GMOCK_INCLUDE_DIRS}, libraries: ${GMOCK_LIBRARIES})")
endif()