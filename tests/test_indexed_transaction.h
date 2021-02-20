#ifndef TOKEN_TEST_INDEXED_TRANSACTION_H
#define TOKEN_TEST_INDEXED_TRANSACTION_H

#include "test_suite.h"
#include "transaction.h"

namespace token{
  class IndexedTransactionTest : public BinaryObjectTest<IndexedTransaction>{
   protected:
    static const std::string kExpectedHash;
    static const int64_t kDefaultIndex;
    static const InputList kDefaultInputs;
    static const OutputList kDefaultOutputs;
    static const Timestamp kDefaultTimestamp;

    IndexedTransactionTest() = default;
    ~IndexedTransactionTest() = default;

    IndexedTransactionPtr GetObject() const{
      return IndexedTransaction::NewInstance(kDefaultIndex, kDefaultInputs, kDefaultOutputs, kDefaultTimestamp);
    }

    IndexedTransactionPtr GetRandomObject() const{
      return IndexedTransaction::NewInstance(kDefaultIndex, kDefaultInputs, kDefaultOutputs, Clock::now());
    }
  };
}

#endif//TOKEN_TEST_INDEXED_TRANSACTION_H