#ifndef TOKEN_TRANSACTION_POOL_H
#define TOKEN_TRANSACTION_POOL_H

#include "transaction.h"

namespace Token{
    class TransactionPoolVisitor;
    class TransactionPool{
    public:
        enum State{
            kUninitialized,
            kInitializing,
            kInitialized,
        };
    private:
        TransactionPool() = delete;

        static void SetState(State state);
    public:
        ~TransactionPool() = delete;

        static size_t GetSize();
        static size_t GetCacheSize();
        static size_t GetMaxCacheSize();
        static State GetState();
        static bool Initialize();
        static void Print(bool cache_only=false);
        static bool Accept(TransactionPoolVisitor* vis);
        static bool RemoveTransaction(const uint256_t& hash);
        static bool PutTransaction(const Handle<Transaction>& tx);
        static bool HasTransaction(const uint256_t& hash);
        static bool GetTransactions(std::vector<uint256_t>& txs);
        static Handle<Transaction> GetTransaction(const uint256_t& hash);

        static inline std::string
        GetPath(){
            return TOKEN_BLOCKCHAIN_HOME + "/transactions";
        }

        static inline bool
        IsUninitialized(){
            return GetState() == kUninitialized;
        }

        static inline bool
        IsInitializing(){
            return GetState() == kInitializing;
        }

        static inline bool
        IsInitialized(){
            return GetState() == kInitialized;
        }
    };

    class TransactionPoolVisitor{
    protected:
        TransactionPoolVisitor() = default;
    public:
        virtual ~TransactionPoolVisitor() = default;
        virtual bool Visit(const Handle<Transaction>& tx) = 0;
    };
}

#endif //TOKEN_TRANSACTION_POOL_H