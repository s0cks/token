#include "test_user.h"

namespace token{
  TEST_F(UserTest, test_eq1){
    User a = User("TestUser");
    ASSERT_EQ(a, a);
  }

  TEST_F(UserTest, test_eq2){
    User a = User("TestUser");
    User b = User("TestUser");
    ASSERT_EQ(a, b);
  }
}