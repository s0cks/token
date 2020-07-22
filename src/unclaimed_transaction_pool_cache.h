#ifndef TOKEN_UNCLAIMED_TRANSACTION_POOL_CACHE_H
#define TOKEN_UNCLAIMED_TRANSACTION_POOL_CACHE_H

#include "uint256_t.h"
#include "unclaimed_transaction_pool.h"

namespace Token{
    class UnclaimedTransactionPoolCache{
        friend class UnclaimedTransactionPool;
    private:
        UnclaimedTransactionPoolCache() = delete;

        static void Evict(const uint256_t& hash);
        static void Promote(const uint256_t& hash);
    public:
        ~UnclaimedTransactionPoolCache() = delete;

        static void EvictLastUsed();
        static void PutTransaction(const uint256_t& hash, const Handle<UnclaimedTransaction>& tx);
        static Handle<UnclaimedTransaction> GetTransaction(const uint256_t& hash);
        static size_t GetSize();
        static bool HasTransaction(const uint256_t& hash);

        static inline bool
        IsFull(){
            return GetSize() == UnclaimedTransactionPool::kMaxPoolSize;
        }
    };
}

#endif //TOKEN_UNCLAIMED_TRANSACTION_POOL_CACHE_H
