#include <gtest/gtest.h>
#include "tests/test_suite.h"
#include "alloc/allocator.h"

int
main(int argc, char** argv){
    using namespace Token;
    ::google::SetStderrLogging(google::INFO);
    ::google::InitGoogleLogging(argv[0]);
    ::testing::InitGoogleTest(&argc, argv);
    Allocator::Initialize();
    return RUN_ALL_TESTS();
}