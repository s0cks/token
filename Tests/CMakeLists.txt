find_package(GTest REQUIRED)

add_executable(token-tests
    main.cc)
target_link_libraries(token-tests
    token
    ${CMAKE_THREAD_LIBS_INIT}
    GTest::gtest
    GTest::gmock
    glog::glog)