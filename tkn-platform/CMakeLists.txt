project(tkn-platform)

find_package(Threads REQUIRED)

add_library(tkn-platform
    include/env.h
    include/filesystem.h
    include/filesystem_linux.h src/filesystem_linux.cc
    include/filesystem_osx.h src/filesystem_osx.cc
    include/filesystem_windows.h src/filesystem_windows.cc
    include/platform.h
    include/os_thread.h
    include/os_thread_linux.h src/os_thread_linux.cc
    include/os_thread_osx.h src/os_thread_osx.cc
    include/os_thread_windows.h src/os_thread_windows.cc)
target_link_libraries(tkn-platform
    ${CMAKE_THREAD_LIBS_INIT} # pthreads
)
target_include_directories(tkn-platform PUBLIC ${PROJECT_SOURCE_DIR}/include)