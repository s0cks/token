#include "test_indexed_transaction.h"

namespace token{
  const std::string IndexedTransactionTest::kExpectedHash = "66687AADF862BD776C8FC18B8E9F8E20089714856EE233B3902A591D0D5F2925";
  const int64_t IndexedTransactionTest::kDefaultIndex = 0;
  const InputList IndexedTransactionTest::kDefaultInputs = {};
  const OutputList IndexedTransactionTest::kDefaultOutputs = {};
  const Timestamp IndexedTransactionTest::kDefaultTimestamp = FromUnixTimestamp(0);

  TEST_BINARY_OBJECT_EQUALS(IndexedTransaction);
  TEST_BINARY_OBJECT_HASH(IndexedTransaction, kExpectedHash);
  TEST_BINARY_OBJECT_WRITE(IndexedTransaction);
}