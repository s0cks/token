#include "test_suite.h"
#include "unclaimed_transaction.h"

namespace Token{
  static inline UnclaimedTransactionPtr
  CreateA(){
    // predictable
    Hash hash = Hash::FromHexString("1191F9F9657BB3AB6D6E043D7B4017507D802795FE58EFA3043A066980C32C72");
    return UnclaimedTransaction::NewInstance(hash, 0, "TestUser", "TestToken");
  }

  static inline UnclaimedTransactionPtr
  CreateB(){
    // unpredictable
    return UnclaimedTransaction::NewInstance(Hash::GenerateNonce(), 0, "TestUser", "TestProduct");
  }

  DEFINE_BINARY_OBJECT_POSITIVE_TEST(UnclaimedTransaction, CreateA);
  DEFINE_BINARY_OBJECT_NEGATIVE_TEST(UnclaimedTransaction, CreateA, CreateB);
  DEFINE_BINARY_OBJECT_HASH_TEST(UnclaimedTransaction, CreateA, "706CCBC77EB2CDA686A5FE4559D876BC0E047E5654B6B82D8F8B5ED8F28E571B");
  DEFINE_BINARY_OBJECT_SERIALIZATION_TEST(UnclaimedTransaction, CreateA);
}