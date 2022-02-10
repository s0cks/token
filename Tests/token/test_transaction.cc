#include <gtest/gtest.h>
#include <glog/logging.h>

#include "helpers.h"
#include "token/transaction.h"

namespace token{
 class TransactionTest : public ::testing::Test{
  public:
   TransactionTest() = default;
   ~TransactionTest() override = default;
 };

 TEST_F(TransactionTest, TestEquality){
   auto timestamp = Clock::now();
   Input inputs[] = {
     Input(sha256::Nonce(1024), 0),
   };
   Output outputs[] = {
     Output("TestUser", "TestProduct"),
   };
   Transaction a(timestamp, inputs, 1, outputs, 1);
   auto aSize = a.GetBufferSize();

   auto data = NewBuffer(aSize);
   ASSERT_TRUE(a.WriteTo(data));
   ASSERT_EQ(data->length(), aSize);

   Transaction b(data);
   ASSERT_TRUE(TimestampsAreEqual(b.timestamp(), a.timestamp()));
   ASSERT_TRUE(InputsEqual(b, inputs, 1));
   ASSERT_TRUE(OutputsEqual(b, outputs, 1));
 }

 class IndexedTransactionTest : public ::testing::Test{
  public:
   IndexedTransactionTest() = default;
   ~IndexedTransactionTest() override = default;
 };

 TEST_F(IndexedTransactionTest, TestEquality){
   auto timestamp = Clock::now();
   Input inputs[] = {
     Input(sha256::Nonce(1024), 0),
   };
   Output outputs[] ={
     Output("TestUser", "TestProduct"),
   };
   IndexedTransaction a(10, timestamp, inputs, 1, outputs, 1);

   auto aSize = a.GetBufferSize();
   auto data = NewBuffer(aSize);
   ASSERT_TRUE(a.WriteTo(data));
   ASSERT_EQ(data->length(), aSize);

   IndexedTransaction b(data);
   ASSERT_TRUE(TimestampsAreEqual(b.timestamp(), a.timestamp()));
   ASSERT_TRUE(InputsEqual(b, inputs, 1));
   ASSERT_TRUE(OutputsEqual(b, outputs, 1));
 }
}