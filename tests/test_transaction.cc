#include "test_transaction.h"

namespace token{
  const std::string TransactionTest::kExpectedHash = "66687AADF862BD776C8FC18B8E9F8E20089714856EE233B3902A591D0D5F2925";
  const InputList TransactionTest::kDefaultInputs = {};
  const OutputList TransactionTest::kDefaultOutputs = {};
  const Timestamp TransactionTest::kDefaultTimestamp = FromUnixTimestamp(0);

  TEST_BINARY_OBJECT_EQUALS(Transaction);
  TEST_BINARY_OBJECT_HASH(Transaction, kExpectedHash);
  TEST_BINARY_OBJECT_WRITE(Transaction);
}