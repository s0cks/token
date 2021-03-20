#ifndef TOKEN_MOCK_WALLET_H
#define TOKEN_MOCK_WALLET_H

#include <gmock/gmock.h>

namespace token{
  class MockWalletManager : public WalletManager{
   public:
    MOCK_METHOD(bool, HasWallet, (const Hash&), (const));
  };
}

#endif//TOKEN_MOCK_WALLET_H