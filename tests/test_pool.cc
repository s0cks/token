#include "test_pool.h"

namespace token{
  const int64_t ObjectPoolTest::kDefaultBlockSize = 1000;
  const std::string ObjectPoolTest::kDefaultBlockHash = "";

  const int64_t ObjectPoolTest::kDefaultTransactionSize = 1000;
  const std::string ObjectPoolTest::kDefaultTransactionHash = "";

  const int64_t ObjectPoolTest::kDefaultUnclaimedTransactionSize = 1000;
  const std::string ObjectPoolTest::kDefaultUnclaimedTransactionHash = "";

#define DEFINE_KEY_TEST(Name) \
  TEST_F(ObjectPoolTest, test_##Name##_key){ \
    ObjectPool::PoolKey a = Create##Name##Key(); \
    ASSERT_EQ(a, a);          \
    ObjectPool::PoolKey b = Create##Name##Key(); \
    ASSERT_EQ(b, b);          \
    ASSERT_EQ(b, a);          \
    ObjectPool::PoolKey c = CreateRandom##Name##Key(); \
    ASSERT_EQ(c, c);          \
    ASSERT_NE(c, b); \
    ASSERT_NE(c, a);          \
    ObjectPool::PoolKey d = CreateRandom##Name##Key(); \
    ASSERT_EQ(d, d);          \
    ASSERT_NE(d, c);          \
    ASSERT_NE(d, b);          \
    ASSERT_NE(d, a);          \
  }
  FOR_EACH_POOL_TYPE(DEFINE_KEY_TEST)
#undef DEFINE_KEY_TEST
}