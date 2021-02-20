#ifndef TOKEN_TEST_INDEXED_TRANSACTION_H
#define TOKEN_TEST_INDEXED_TRANSACTION_H

#include "test_suite.h"
#include "transaction.h"

namespace token{
 class TestIndexedTransaction : public ::testing::Test{
  protected:
   IndexedTransactionPtr itx1;
   IndexedTransactionPtr itx2;

   void SetUp(){
     InputList inputs = {};
     OutputList outputs = {};
     int64_t index = 0;

     itx1 = IndexedTransaction::NewInstance(index, inputs, outputs, FromUnixTimestamp(0));
     itx2 = IndexedTransaction::NewInstance(index, inputs, outputs);
   }

   void TearDown(){

   }
  public:
   TestIndexedTransaction() = default;
   ~TestIndexedTransaction() = default;
 };
}

#endif//TOKEN_TEST_INDEXED_TRANSACTION_H