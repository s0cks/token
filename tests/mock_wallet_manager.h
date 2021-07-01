#ifndef TOKEN_MOCK_WALLET_H
#define TOKEN_MOCK_WALLET_H

#include <gmock/gmock.h>
#include "wallet_manager.h"

namespace token{
  class MockWalletManager : public WalletManager{
   public:
    MOCK_METHOD(bool, HasWallet, (const User&), (const));
    MOCK_METHOD(bool, RemoveWallet, (const User&), (const));
    MOCK_METHOD(bool, PutWallet, (const User&, const Wallet&), (const));
    MOCK_METHOD(Wallet, GetUserWallet, (const User&), (const));
    MOCK_METHOD(bool, GetWallet, (const User&, const Wallet&), (const));
    MOCK_METHOD(int64_t, GetNumberOfWallets, (), (const));
  };

  static inline WalletManagerPtr
  NewMockWalletManager(){
    return std::make_shared<MockWalletManager>();
  }
}

#endif//TOKEN_MOCK_WALLET_H