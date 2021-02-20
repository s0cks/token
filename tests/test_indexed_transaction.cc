#include "test_indexed_transaction.h"

namespace token{
  const std::string IndexedTransactionTest::kExpectedHash = "2C34CE1DF23B838C5ABF2A7F6437CCA3D3067ED509FF25F11DF6B11B582B51EB";
  const int64_t IndexedTransactionTest::kDefaultIndex = 0;
  const InputList IndexedTransactionTest::kDefaultInputs = {};
  const OutputList IndexedTransactionTest::kDefaultOutputs = {};
  const Timestamp IndexedTransactionTest::kDefaultTimestamp = FromUnixTimestamp(0);

  TEST_BINARY_OBJECT_EQUALS(IndexedTransaction);
  TEST_BINARY_OBJECT_HASH(IndexedTransaction, kExpectedHash);
  TEST_BINARY_OBJECT_WRITE(IndexedTransaction);
}