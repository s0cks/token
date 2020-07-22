#ifndef TOKEN_BLOCK_POOL_CACHE_H
#define TOKEN_BLOCK_POOL_CACHE_H

#include "block_pool.h"

namespace Token{
    class BlockPoolCache{
        friend class BlockPool;
    private:
        BlockPoolCache() = delete;

        static void Evict(const uint256_t& hash);
        static void Promote(const uint256_t& hash);
    public:
        ~BlockPoolCache() = delete;

        static void EvictLastUsed();
        static void PutTransaction(const uint256_t& hash, const Handle<Block>& tx);
        static Handle<Block> GetTransaction(const uint256_t& hash);
        static size_t GetSize();
        static bool HasTransaction(const uint256_t& hash);

        static inline bool
        IsFull(){
            return GetSize() == BlockPool::kMaxPoolSize;
        }
    };
}

#endif //TOKEN_BLOCK_POOL_CACHE_H