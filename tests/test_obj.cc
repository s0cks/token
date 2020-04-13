#include <glog/logging.h>
#include <gtest/gtest.h>
#include "object.h"

namespace Token{
    TEST(object_test_suite, hash_test){
        Hash* hash = new Hash("Test");
        ASSERT_EQ(hash->GetHash(), "Test");
    }
}