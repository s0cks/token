include(FindPackageHandleStandardArgs)

find_path(
   CRYPTOPP_INCLUDE_DIR
   NAMES cryptopp/cryptlib.h
   PATHS /usr/include;/usr/local/include
)
find_library(
   CRYPTOPP_LIBRARY
   NAMES cryptopp
   PATHS /usr/lib;/usr/local/lib
)

find_package_handle_standard_args(cryptopp DEFAULT_MSG CRYPTOPP_INCLUDE_DIR CRYPTOPP_LIBRARY)

if(CRYPTOPP_FOUND)
  set(CRYPTOPP_INCLUDE_DIRS ${CRYPTOPP_INCLUDE_DIR})
  set(CRYPTOPP_LIBRARIES ${CRYPTOPP_LIBRARY})
  message(STATUS "Found cryptopp (include: ${CRYPTOPP_INCLUDE_DIRS}, libraries: ${CRYPTOPP_LIBRARIES})")
endif()