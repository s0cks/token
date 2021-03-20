include(FindPackageHandleStandardArgs)
find_path(
   GTEST_INCLUDE_DIR
   NAMES gtest/gtest.h
   PATHS /usr/include;/usr/local/include
)
find_library(
   GTEST_LIBRARY
   NAMES gtest
   PATHS /usr/lib;/usr/local/lib
)

find_package_handle_standard_args(gtest DEFAULT_MSG GTEST_INCLUDE_DIR GTEST_LIBRARY)
if(GTEST_FOUND)
  set(GTEST_INCLUDE_DIRS ${GTEST_INCLUDE_DIR})
  set(GTEST_LIBRARIES ${GTEST_LIBRARY})
  message(STATUS "Found gtest (include: ${GTEST_INCLUDE_DIRS}, libraries: ${GTEST_LIBRARIES})")
endif()