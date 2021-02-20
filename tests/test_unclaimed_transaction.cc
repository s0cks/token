#include "test_unclaimed_transaction.h"

namespace token{
  const std::string UnclaimedTransactionTest::kExpectedHash = "1191F9F9657BB3AB6D6E043D7B4017507D802795FE58EFA3043A066980C32C72";
  const TransactionReference UnclaimedTransactionTest::kDefaultTransactionReference = TransactionReference(Hash::GenerateNonce(), 0);
  const std::string UnclaimedTransactionTest::kDefaultUser = "TestUser";
  const std::string UnclaimedTransactionTest::kDefaultToken = "TestToken";

  TEST_BINARY_OBJECT_HASH(UnclaimedTransaction, kExpectedHash);
  TEST_BINARY_OBJECT_EQUALS(UnclaimedTransaction);
  TEST_BINARY_OBJECT_WRITE(UnclaimedTransaction);
}