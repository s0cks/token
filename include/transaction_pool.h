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

        static const size_t kMaxPoolSize = 128;
    private:
        TransactionPool() = delete;

        static void SetState(State state);
    public:
        ~TransactionPool() = delete;

        static State GetState();
        static void Initialize();
        static void Accept(TransactionPoolVisitor* vis);
        static void RemoveTransaction(const uint256_t& hash);
        static void PutTransaction(const Handle<Transaction>& tx);
        static bool HasTransaction(const uint256_t& hash);
        static bool GetTransactions(std::vector<uint256_t>& txs);
        static Handle<Transaction> GetTransaction(const uint256_t& hash);
        static size_t GetNumberOfTransactions();

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