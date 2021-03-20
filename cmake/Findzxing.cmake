include(FindPackageHandleStandardArgs)
find_path(
   ZXING_INCLUDE_DIR
   NAMES ZXing
   PATHS /usr/include;/usr/local/include
)
find_library(
   ZXING_LIBRARY
   NAMES ZXing libZXing
   PATHS /usr/lib;/usr/local/lib
)

find_package_handle_standard_args(zxing DEFAULT_MSG ZXING_INCLUDE_DIR ZXING_LIBRARY)
if(ZXING_FOUND)
  set(ZXING_INCLUDE_DIRS ${ZXING_INCLUDE_DIR})
  set(ZXING_LIBRARIES ${ZXING_LIBRARY})
  message(STATUS "Found zxing (include: ${ZXING_INCLUDE_DIRS}, libraries: ${ZXING_LIBRARIES})")
endif()