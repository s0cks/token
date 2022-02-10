#include <gtest/gtest.h>
#include "token/user.h"
#include "token/buffer.h"

#include "helpers.h"

namespace token{
 class UserTest : public ::testing::Test{
  public:
   UserTest() = default;
   ~UserTest() override = default;

   static constexpr const char* kTestUserName = "TestUser";
 };

 TEST_F(UserTest, TestEquality){
   User user(kTestUserName);
   ASSERT_TRUE(IsUser(user, kTestUserName));
   ASSERT_FALSE(IsUser(user, "Test"));
 }

 TEST_F(UserTest, TestSerialization){
   User user(kTestUserName);
   auto data = NewBuffer(User::kSize);
   ASSERT_TRUE(data->PutUser(user));
   ASSERT_EQ(data->GetUser(), user);
 }
}