#include "test_suite.h"
#include "wallet.h"

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
}