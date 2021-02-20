#include "test_indexed_transaction.h"

namespace token{
  TEST_F(TestIndexedTransaction, test_pos){
    ASSERT_TRUE(itx1->Equals(itx1));
  }

  TEST_F(TestIndexedTransaction, test_neg){
    ASSERT_FALSE(itx1->Equals(itx2));
  }

  TEST_F(TestIndexedTransaction, test_hash){
    ASSERT_EQ(itx1->GetHash(), Hash::FromHexString("2C34CE1DF23B838C5ABF2A7F6437CCA3D3067ED509FF25F11DF6B11B582B51EB"));
  }

  TEST_F(TestIndexedTransaction, test_write){
    BufferPtr buffer = Buffer::NewInstance(itx1->GetBufferSize());
    ASSERT_TRUE(itx1->Write(buffer));
    IndexedTransactionPtr itx3 = IndexedTransaction::FromBytes(buffer);
    ASSERT_TRUE(itx3->Equals(itx1));
  }
}