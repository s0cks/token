include(FindPackageHandleStandardArgs)

find_path(
   PNG_INCLUDE_DIR
   NAMES png.h
   PATHS /usr/include;/usr/local/include
)

find_library(
   PNG_LIBRARY
   NAMES png
   PATHS /usr/lib;/usr/local/lib
)

find_package_handle_standard_args(png DEFAULT_MSG PNG_INCLUDE_DIR PNG_LIBRARY)

if(PNG_FOUND)
  set(PNG_INCLUDE_DIRS ${PNG_INCLUDE_DIR})
  set(PNG_LIBRARIES ${PNG_LIBRARY})
  message(STATUS "Found libpng (include: ${PNG_INCLUDE_DIRS}, libraries: ${PNG_LIBRARIES})")
endif()