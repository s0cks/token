#include "test_wallet.h"

namespace token{
  static inline bool
  GenerateWallet(Wallet& wallet, int size){
    for(int idx = 0; idx < size; idx++)
      if(!wallet.insert(Hash::GenerateNonce()).second)
        return false;
    return true;
  }

  TEST(TestWallet, test_eq){
    Wallet a;
    ASSERT_TRUE(GenerateWallet(a, 10000));
    ASSERT_EQ(a, a);
  }

  TEST(TestWallet, test_ne){
    Wallet a;
    ASSERT_TRUE(GenerateWallet(a, 10000));

    Wallet b;
    ASSERT_TRUE(GenerateWallet(b, 10000));
    ASSERT_NE(a, b);
  }

  TEST(TestWallet, test_serialization){
    Wallet a;
    ASSERT_TRUE(GenerateWallet(a, 10000));

    BufferPtr buff = Buffer::NewInstance(GetBufferSize(a));
    ASSERT_TRUE(Encode(a, buff));

    Wallet b;
    ASSERT_TRUE(Decode(buff, b));
    ASSERT_EQ(a, b);
  }

  static inline bool
  GenerateRandomWallet(Wallet& wallet, const int64_t& size){
    for(int64_t i = 0; i < size; i++){
      if(!wallet.insert(Hash::GenerateNonce()).second)
        return false;
    }
    return true;
  }

  TEST_F(WalletManagerTest, test_put_wallet){
    // generate random wallet
    Wallet wallet;
    ASSERT_TRUE(GenerateRandomWallet(wallet, 25));
    ASSERT_EQ(wallet.size(), 25);

    // test that put wallet is functioning
    ASSERT_TRUE(WalletManager::GetInstance()->PutWallet((const std::string&)"TestUser", wallet));
  }

  TEST_F(WalletManagerTest, test_has_wallet){
    // generate random wallet
    Wallet wallet;
    ASSERT_TRUE(GenerateRandomWallet(wallet, 25));
    ASSERT_EQ(wallet.size(), 25);

    // test that has wallet is functioning
    ASSERT_TRUE(WalletManager::GetInstance()->PutWallet((const std::string&)"TestUser", wallet));
    ASSERT_TRUE(WalletManager::GetInstance()->HasWallet((const std::string&)"TestUser"));
  }

  TEST_F(WalletManagerTest, test_get_wallet){
    // generate random wallet
    Wallet wallet;
    ASSERT_TRUE(GenerateRandomWallet(wallet, 25));
    ASSERT_EQ(wallet.size(), 25);

    // test that put wallet is functioning
    ASSERT_TRUE(WalletManager::GetInstance()->PutWallet((const std::string&)"TestUser", wallet));

    Wallet wallet2;
    ASSERT_TRUE(WalletManager::GetInstance()->GetWallet((const std::string&)"TestUser", wallet2));
    ASSERT_EQ(wallet, wallet2);
  }

  TEST_F(WalletManagerTest, test_remove_wallet){
    // generate random wallet
    Wallet wallet;
    ASSERT_TRUE(GenerateRandomWallet(wallet, 25));
    ASSERT_EQ(wallet.size(), 25);

    // test that has wallet is functioning
    ASSERT_TRUE(WalletManager::GetInstance()->PutWallet((const std::string&)"TestUser", wallet));
    ASSERT_TRUE(WalletManager::GetInstance()->RemoveWallet((const std::string&)"TestUser"));
  }
}