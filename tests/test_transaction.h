#ifndef TOKEN_TEST_TRANSACTION_H
#define TOKEN_TEST_TRANSACTION_H

#include "test_suite.h"
#include "transaction.h"

namespace token{
  class TransactionTest : public BinaryObjectTest<Transaction>{
   protected:
    static const std::string kExpectedHash;
    static const InputList kDefaultInputs;
    static const OutputList kDefaultOutputs;
    static const Timestamp kDefaultTimestamp;

    TransactionTest() = default;

    TransactionPtr GetObject() const{
      return Transaction::NewInstance(kDefaultInputs, kDefaultOutputs, kDefaultTimestamp);
    }

    TransactionPtr GetRandomObject() const{
      return Transaction::NewInstance(kDefaultInputs, kDefaultOutputs, Clock::now());
    }
   public:
    ~TransactionTest() = default;
  };
}

#endif//TOKEN_TEST_TRANSACTION_H