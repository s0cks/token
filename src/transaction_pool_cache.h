#ifndef TOKEN_TRANSACTION_POOL_CACHE_H
#define TOKEN_TRANSACTION_POOL_CACHE_H

#include <transaction_pool.h>
#include "uint256_t.h"
#include "transaction.h"

namespace Token{
    class TransactionPoolCache{
        friend class TransactionPool;
    private:
        TransactionPoolCache() = delete;

        static void Evict(const uint256_t& hash);
        static void Promote(const uint256_t& hash);
    public:
        ~TransactionPoolCache() = delete;

        static void EvictLastUsed();
        static void PutTransaction(const uint256_t& hash, const Handle<Transaction>& tx);
        static Handle<Transaction> GetTransaction(const uint256_t& hash);
        static size_t GetSize();
        static bool HasTransaction(const uint256_t& hash);

        static inline bool
        IsFull(){
            return GetSize() == TransactionPool::kMaxPoolSize;
        }
    };
}

#endif //TOKEN_TRANSACTION_POOL_CACHE_H