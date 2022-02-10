#include <gtest/gtest.h>
#include <glog/logging.h>

#include "helpers.h"
#include "token/input.h"

namespace token{
 TEST(InputTest, TestEquals){
   TransactionReference source(sha256::Nonce(1024), 10);

   Input a(source);
   Input b(source);
   ASSERT_EQ(a, b);
 }

 TEST(InputTest, TestNotEquals){
   Input a(sha256::Nonce(1024), 10);
   Input b(sha256::Nonce(1024), 10);
   ASSERT_NE(a, b);
 }

 TEST(InputTest, TestSerialize){
   TransactionReference source(sha256::Nonce(1024), 10);
   Input a(source);

   auto data = NewBuffer(Input::kSize);
   ASSERT_TRUE(a.WriteTo(data));
   ASSERT_EQ(data->length(), Input::kSize);

   Input b(data);
   ASSERT_TRUE(IsInput(b, source));
 }
}