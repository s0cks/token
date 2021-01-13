########################################################################
# CMake module for finding ZXing-cpp
#
# The following variables will be defined:
#
#   - ZXING_FOUND
#   - ZXING_INCLUDE_DIR
#   - ZXING_LIBRARIES
FIND_PATH(
  ZXING_INCLUDE_DIR NAMES ZXing
  PATHS /usr/local/include /usr/include
  DOC "Path in which the file ZXing/ZXing.h is located."
)
FIND_LIBRARY(
  ZXING_LIBRARIES
  NAMES ZXing libZXing
)
INCLUDE(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(
  ZXing DEFAULT_MSG
  ZXING_LIBRARIES ZXING_INCLUDE_DIR
)