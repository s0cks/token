project(tkn-utils)

add_library(tkn-utils include/platform.h include/relaxed_atomic.h include/os_thread.h include/os_thread_linux.h include/os_thread_osx.h include/os_thread_windows.h src/os_thread_linux.cc)

target_include_directories(tkn-utils PUBLIC ${PROJECT_SOURCE_DIR}/include)