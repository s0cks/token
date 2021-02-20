#include "test_transaction.h"

namespace token{
  TEST_F(TestTransaction, test_pos){
    ASSERT_TRUE(tx1->Equals(tx1));
  }

  TEST_F(TestTransaction, test_neg){
    ASSERT_FALSE(tx1->Equals(tx2));
  }

  TEST_F(TestTransaction, test_hash){
    ASSERT_EQ(tx1->GetHash(), Hash::FromHexString("66687AADF862BD776C8FC18B8E9F8E20089714856EE233B3902A591D0D5F2925"));
  }

  TEST_F(TestTransaction, test_write){
    BufferPtr buffer = Buffer::NewInstance(tx1->GetBufferSize());
    ASSERT_TRUE(tx1->Write(buffer));
    TransactionPtr tx3 = Transaction::FromBytes(buffer);
    ASSERT_TRUE(tx3->Equals(tx1));
  }
}