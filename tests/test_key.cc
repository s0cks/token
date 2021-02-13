#include "test_suite.h"
#include "key.h"

namespace token{
  TEST(TestKey, test_blk_key){
    BlockPtr blk = Block::Genesis();

    BlockKey k1(blk);
    ASSERT_EQ(k1, k1);

    BlockKey k2(blk);
    ASSERT_EQ(k2, k2);
    ASSERT_EQ(k2, k1);

    BlockKey k3(17497, 18923, Hash::GenerateNonce());
    ASSERT_EQ(k3, k3);
    ASSERT_NE(k3, k2);
    ASSERT_NE(k3, k1);
  }
}