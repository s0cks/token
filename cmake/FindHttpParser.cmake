########################################################################
# CMake module for finding HTTPPARSER
#
# The following variables will be defined:
#
#  HTTPPARSER_FOUND
#  HTTPPARSER_INCLUDE_DIR
#  HTTPPARSER_LIBRARIES
#

FIND_PATH(
        HTTPPARSER_INCLUDE_DIR
        NAMES http_parser.h
)
FIND_LIBRARY(
        HTTPPARSER_LIBRARIES
        NAMES http_parser libhttp_parser
)

INCLUDE(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(HTTP_PARSER DEFAULT_MSG HTTPPARSER_LIBRARIES HTTPPARSER_INCLUDE_DIR)