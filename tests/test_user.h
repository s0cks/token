#ifndef TOKEN_TEST_USER_H
#define TOKEN_TEST_USER_H

#include "user.h"
#include "test_suite.h"

namespace token{
  class UserTest : public ::testing::Test{
   protected:
    UserTest() = default;
   public:
    ~UserTest() = default;
  };
}

#endif//TOKEN_TEST_USER_H