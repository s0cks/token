#ifndef TOKEN_TEST_TEST_H
#define TOKEN_TEST_TEST_H

#include <gtest/gtest.h>
#include <glog/logging.h>
#include "token.h"

namespace Token{
    class TestTest : public ::testing::Test{
    public:
        void SetUp(){
            LOG(INFO) << "Version: " << Token::GetVersion();
        }
    };
}

#endif //TOKEN_TEST_TEST_H