include(FindPackageHandleStandardArgs)
find_path(
   UNWIND_INCLUDE_DIR
   NAMES unwind unwind.h
   PATHS /usr/include /usr/local/include
)
find_library(
   UNWIND_LIBRARY
   NAMES unwind libunwind
   PATHS /usr/lib /usr/local/lib
)

find_package_handle_standard_args(unwind DEFAULT_MSG UNWIND_INCLUDE_DIR UNWIND_LIBRARY)

if(UV_FOUND)
  set(UNWIND_INCLUDE_DIRS ${UNWIND_INCLUDE_DIR})
  set(UNWIND_LIBRARIES ${UNWIND_LIBRARY})
  message(STATUS "Found libunwind (include: ${UNWIND_INCLUDE_DIRS}, libraries: ${UNWIND_LIBRARIES})")
endif()