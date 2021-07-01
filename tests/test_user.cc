#include "test_user.h"

namespace token{
  TEST_F(UserTest, TestEq){
    User a = User("TestUser");
    User b = User("TestUser");
    ASSERT_EQ(a, b);
  }
}