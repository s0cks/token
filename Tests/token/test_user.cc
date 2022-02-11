#include <gtest/gtest.h>
#include "token/user.h"
#include "token/buffer.h"

#include "helpers.h"

namespace token{
 class UserTest : public ::testing::Test{
  public:
   UserTest() = default;
   ~UserTest() override = default;

   static constexpr const char* kUser1 = "TestUser";
   static constexpr const char* kUser2 = "Test";
 };

 TEST_F(UserTest, TestEquality){
   User user(kUser1);
   ASSERT_TRUE(IsUser(user, kUser1));
   ASSERT_FALSE(IsUser(user, kUser2));
 }

 TEST_F(UserTest, TestSerialization){
   User user(kUser1);
   auto data = NewBuffer(User::kSize);
   ASSERT_TRUE(data->PutUser(user));
   ASSERT_EQ(data->GetUser(), user);
 }
}