#ifndef TOKEN_TEST_UNCLAIMED_TRANSACTION_H
#define TOKEN_TEST_UNCLAIMED_TRANSACTION_H

#include "test_suite.h"
#include "unclaimed_transaction.h"

namespace token{
  class UnclaimedTransactionTest : public BinaryObjectTest<UnclaimedTransaction>{
   protected:
    static const std::string kExpectedHash;
    static const TransactionReference kDefaultTransactionReference;
    static const std::string kDefaultUser;
    static const std::string kDefaultToken;

    UnclaimedTransactionTest() = default;

    UnclaimedTransactionPtr GetObject() const{
      return UnclaimedTransaction::NewInstance(kDefaultTransactionReference, kDefaultUser, kDefaultToken);
    }

    UnclaimedTransactionPtr GetRandomObject() const{
      return UnclaimedTransaction::NewInstance(TransactionReference(Hash::GenerateNonce(), 0), kDefaultUser, kDefaultToken);
    }
   public:
    ~UnclaimedTransactionTest() = default;
  };
}

#endif//TOKEN_TEST_UNCLAIMED_TRANSACTION_H