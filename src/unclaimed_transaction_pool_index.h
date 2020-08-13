#ifndef TOKEN_UNCLAIMED_TRANSACTION_POOL_INDEX_H
#define TOKEN_UNCLAIMED_TRANSACTION_POOL_INDEX_H

#include "common.h"
#include "unclaimed_transaction.h"

namespace Token{
    class UnclaimedTransactionPoolIndex{
    public:
        UnclaimedTransactionPoolIndex() = delete;
        ~UnclaimedTransactionPoolIndex() = delete;

        static void Initialize();
        static Handle<UnclaimedTransaction> GetData(const uint256_t& hash);
        static size_t GetNumberOfUnclaimedTransactions();
        static bool RemoveData(const uint256_t& hash);
        static bool PutData(const Handle<UnclaimedTransaction>& utxo);
        static bool HasData(const uint256_t& hash);

        static inline bool HasData(){
            return GetNumberOfUnclaimedTransactions() > 0;
        }

        static std::string GetDirectory();
    };
}

#endif //TOKEN_UNCLAIMED_TRANSACTION_POOL_INDEX_H