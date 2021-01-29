#include "test_suite.h"
#include "transaction.h"

namespace Token{
  static TransactionPtr
  CreateA(){
    // predictable
    InputList inputs = {};
    OutputList outputs = {};
    return Transaction::NewInstance(0, inputs, outputs, FromUnixTimestamp(0));
  }

  static TransactionPtr
  CreateB(){
    // unpredictable
    InputList inputs = {};
    OutputList outputs = {};
    return Transaction::NewInstance(0, inputs, outputs);
  }

  DEFINE_BINARY_OBJECT_POSITIVE_TEST(Transaction, CreateA);
  DEFINE_BINARY_OBJECT_NEGATIVE_TEST(Transaction, CreateA, CreateB);
  DEFINE_BINARY_OBJECT_HASH_TEST(Transaction, CreateA, "66687AADF862BD776C8FC18B8E9F8E20089714856EE233B3902A591D0D5F2925");
  DEFINE_BINARY_OBJECT_SERIALIZATION_TEST(Transaction, CreateA);
}