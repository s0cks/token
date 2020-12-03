#include <gtest/gtest.h>
#include "tests/test_suite.h"

int
main(int argc, char** argv){
    ::google::SetStderrLogging(google::INFO);
    ::google::InitGoogleLogging(argv[0]);
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}