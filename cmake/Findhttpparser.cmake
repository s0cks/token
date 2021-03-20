include(FindPackageHandleStandardArgs)

find_path(
   HTTPPARSER_INCLUDE_DIR
   NAMES http_parser.h
   PATHS /usr/include;/usr/local/include
)
find_library(
   HTTPPARSER_LIBRARY
   NAMES http_parser libhttp_parser
   PATHS /usr/lib;/usr/local/lib
)

find_package_handle_standard_args(httpparser DEFAULT_MSG HTTPPARSER_LIBRARY HTTPPARSER_INCLUDE_DIR)

if(HTTPPARSER_FOUND)
  set(HTTPPARSER_INCLUDE_DIRS ${HTTPPARSER_INCLUDE_DIR})
  set(HTTPPARSER_LIBRARIES ${HTTPPARSER_LIBRARY})
  message(STATUS "Found http-parser (include: ${HTTPPARSER_INCLUDE_DIRS}, libraries: ${HTTPPARSER_LIBRARIES})")
endif()