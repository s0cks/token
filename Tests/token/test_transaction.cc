#include <gtest/gtest.h>
#include <glog/logging.h>

#include "helpers.h"
#include "token/transaction.h"

namespace token{
 class TransactionTest : public ::testing::Test{
   static const Timestamp kTransaction1Timestamp;

   static const uint256 kTransaction1Input1Hash;
   static const uint64_t kTransaction1Input1Index;

   static const Input kTransaction1Inputs[];

   static Transaction* kTransaction1;
  protected:
   void SetUp() override{
     kTransaction1 = new Transaction(kTransaction1Timestamp, kTransaction1Inputs, 1, nullptr, 0);
   }
  public:
   TransactionTest() = default;
   ~TransactionTest() override = default;
 };

 const Timestamp TransactionTest::kTransaction1Timestamp = Clock::now();

 const uint256 TransactionTest::kTransaction1Input1Hash = sha256::Nonce();
 const uint64_t TransactionTest::kTransaction1Input1Index = 10;

 const Input TransactionTest::kTransaction1Inputs[] = {
   Input(kTransaction1Input1Hash, kTransaction1Input1Index),
 };

 Transaction* TransactionTest::kTransaction1 = nullptr;

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