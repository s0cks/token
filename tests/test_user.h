#ifndef TOKEN_TEST_USER_H
#define TOKEN_TEST_USER_H

#include "test_suite.h"

namespace token{
  class UserTest : public ::testing::Test{
   public:
    UserTest() = default;
    virtual ~UserTest() override = default;
  };
}

#endif//TOKEN_TEST_USER_H