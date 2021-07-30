#include <gtest/gtest.h>
#include "transaction_output.h"

namespace token{
  TEST(OutputTest, TestSerialization){
    auto user = "TestUser";
    auto product = "TestToken";

    Output a(user, product);
    LOG(INFO) << "user: " << user;
    LOG(INFO) << "product: " << product;

    auto data = a.ToBuffer();
    Output b(data);

    ASSERT_EQ(a, b);
    ASSERT_EQ(a.user(), User(user));
    ASSERT_EQ(a.product(), Product(product));
  }
}