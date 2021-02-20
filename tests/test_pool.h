#ifndef TOKEN_TEST_POOL_H
#define TOKEN_TEST_POOL_H

#include "pool.h"
#include "test_suite.h"

namespace token{
  class ObjectPoolTest : public ::testing::Test{
   protected:
    static const int64_t kDefaultBlockSize;
    static const std::string kDefaultBlockHash;

    static const int64_t kDefaultTransactionSize;
    static const std::string kDefaultTransactionHash;

    static const int64_t kDefaultUnclaimedTransactionSize;
    static const std::string kDefaultUnclaimedTransactionHash;

    ObjectPoolTest() = default;

#define DEFINE_CREATE_DEFAULT_KEY(Name) \
    inline ObjectPool::PoolKey Create##Name##Key() const{ return ObjectPool::PoolKey(Type::k##Name, kDefault##Name##Size, Hash::FromHexString(kDefault##Name##Hash)); }
#define DEFINE_CREATE_RANDOM_KEY(Name) \
    inline ObjectPool::PoolKey CreateRandom##Name##Key() const{ return ObjectPool::PoolKey(Type::k##Name, kDefault##Name##Size, Hash::GenerateNonce()); }
#define DEFINE_CREATE_KEY(Name) \
    DEFINE_CREATE_RANDOM_KEY(Name) \
    DEFINE_CREATE_DEFAULT_KEY(Name)

    FOR_EACH_POOL_TYPE(DEFINE_CREATE_KEY)
#undef DEFINE_CREATE_KEY
#undef DEFINE_CREATE_RANDOM_KEY
#undef DEFINE_CREATE_DEFAULT_KEY
   public:
    ~ObjectPoolTest() = default;
  };
}

#endif//TOKEN_TEST_POOL_H