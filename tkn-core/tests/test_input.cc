#include "gtest/gtest.h"

#include "transaction_input.h"

namespace token{
  TEST(InputTest, TestSerialization){
    auto hash = Hash::GenerateNonce();
    auto index = 0;
    auto user = "TestUser";

    Input a(hash, index, user);
    LOG(INFO) << "transaction_hash: " << hash;
    LOG(INFO) << "transaction_index: " << index;
    LOG(INFO) << "user: " << user;

    auto data = a.ToBuffer();
    Input b(data);

    ASSERT_EQ(a, b);
    ASSERT_EQ(b.transaction(), hash);
    ASSERT_EQ(b.index(), index);
    ASSERT_EQ(b.user(), User(user));
  }
}