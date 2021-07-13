project(tkn-http CXX)
set(CMAKE_CXX_STANDARD 11)

add_library(tkn-http STATIC
        include/http/http_common.h
        include/http/http_client.h src/http/http_client.cc
        include/http/http_controller.h
        include/http/http_message.h src/http/http_message.cc
        include/http/http_request.h src/http/http_request.cc
        include/http/http_response.h
        include/http/http_router.h src/http/http_router.cc
        include/http/http_service.h
        include/http/http_session.h src/http/http_session.cc)
target_link_libraries(tkn-http tkn-utils)
target_include_directories(tkn-http PUBLIC ${PROJECT_SOURCE_DIR}/include)