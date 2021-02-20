#include "test_unclaimed_transaction.h"

namespace token{
  const std::string UnclaimedTransactionTest::kExpectedHash = "1CBBC63F6CC66AF5F1F716CAAD8066EEF4E8E0F1F99D364D7E751CA6B1A16110";
  const TransactionReference UnclaimedTransactionTest::kDefaultTransactionReference = TransactionReference(Hash::FromHexString(kExpectedHash), 0);
  const std::string UnclaimedTransactionTest::kDefaultUser = "TestUser";
  const std::string UnclaimedTransactionTest::kDefaultToken = "TestToken";

  TEST_BINARY_OBJECT_HASH(UnclaimedTransaction, kExpectedHash);
  TEST_BINARY_OBJECT_EQUALS(UnclaimedTransaction);
  TEST_BINARY_OBJECT_WRITE(UnclaimedTransaction);
}