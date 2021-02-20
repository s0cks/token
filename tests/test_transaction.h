#ifndef TOKEN_TEST_TRANSACTION_H
#define TOKEN_TEST_TRANSACTION_H

#include "test_suite.h"
#include "transaction.h"

namespace token{
  class TestTransaction : public ::testing::Test{
   protected:
    TransactionPtr tx1;
    TransactionPtr tx2;

    void SetUp(){
      InputList inputs = {};
      OutputList outputs = {};

      tx1 = Transaction::NewInstance(inputs, outputs, FromUnixTimestamp(0));
      tx2 = Transaction::NewInstance(inputs, outputs);
    }

    void TearDown(){}
   public:
    TestTransaction() = default;
    ~TestTransaction() = default;
  };
}

#endif//TOKEN_TEST_TRANSACTION_H