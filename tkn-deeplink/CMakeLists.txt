project(tkn-deeplink CXX)
set(CMAKE_CXX_STANDARD 11)

add_library(tkn-deeplink STATIC
    include/deeplink.h src/deeplink.cc)
target_link_libraries(tkn-deeplink tkn-utils)
target_include_directories(tkn-deeplink PUBLIC ${PROJECT_SOURCE_DIR}/include)