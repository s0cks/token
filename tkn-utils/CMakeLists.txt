project(tkn-utils CXX)
set(CMAKE_CXX_STANDARD 11)

add_library(tkn-utils STATIC
        include/atomic/relaxed_atomic.h
        include/atomic/work_stealing_queue.h
        include/placeholder.h src/placeholder.cc)
target_include_directories(tkn-utils PUBLIC ${PROJECT_SOURCE_DIR}/include)