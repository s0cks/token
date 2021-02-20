#include "test_transaction.h"

namespace token{
  const std::string TransactionTest::kExpectedHash = "9D908ECFB6B256DEF8B49A7C504E6C889C4B0E41FE6CE3E01863DD7B61A20AA0";
  const InputList TransactionTest::kDefaultInputs = {};
  const OutputList TransactionTest::kDefaultOutputs = {};
  const Timestamp TransactionTest::kDefaultTimestamp = FromUnixTimestamp(0);

  TEST_BINARY_OBJECT_EQUALS(Transaction);
  TEST_BINARY_OBJECT_HASH(Transaction, kExpectedHash);
  TEST_BINARY_OBJECT_WRITE(Transaction);
}