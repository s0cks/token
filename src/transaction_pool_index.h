#ifndef TOKEN_TRANSACTION_POOL_INDEX_H
#define TOKEN_TRANSACTION_POOL_INDEX_H

#include <leveldb/db.h>
#include "transaction.h"

namespace Token{
    class TransactionPoolIndex{
    public:
        TransactionPoolIndex() = delete;
        ~TransactionPoolIndex() = delete;

        static void Initialize();
        static void PutData(const Handle<Transaction>& tx);
        static void RemoveData(const uint256_t& hash);
        static Handle<Transaction> GetData(const uint256_t& hash);
        static size_t GetNumberOfTransactions();
        static bool HasData(const uint256_t& hash);

        static bool HasData(){
            return GetNumberOfTransactions() > 0;
        }

        static std::string GetDirectory();
    };
}

#endif //TOKEN_TRANSACTION_POOL_INDEX_H