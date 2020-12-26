#include <gtest/gtest.h>
#include <glog/logging.h>
#include "common.h"

namespace Token{
    TEST(TestTest, test_test){
        ASSERT_EQ(Token::GetVersion(), "1.0.0");
    }
}