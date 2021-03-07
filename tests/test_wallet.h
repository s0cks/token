#ifndef TOKEN_TEST_WALLET_H
#define TOKEN_TEST_WALLET_H

#include "wallet.h"
#include "test_suite.h"

namespace token{
  class WalletTest : public ::testing::Test{
   protected:
    WalletTest() = default;
   public:
    ~WalletTest() = default;
  };

  class WalletManagerTest : public ::testing::Test{
   protected:
    WalletManagerTest() = default;

    void SetUp(){

    }

    void TearDown(){

    }
   public:
    ~WalletManagerTest() = default;
  };
}

#endif//TOKEN_TEST_WALLET_H