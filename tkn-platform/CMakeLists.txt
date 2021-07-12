project(tkn-platform)

add_library(tkn-platform
        include/platform.h
        include/os_thread.h
        include/os_thread_linux.h src/os_thread_linux.cc
        include/os_thread_osx.h src/os_thread_osx.cc
        include/os_thread_windows.h src/os_thread_windows.cc)
target_include_directories(tkn-platform PUBLIC ${PROJECT_SOURCE_DIR}/include)