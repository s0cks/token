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
        static State GetState();
        static bool Print();
        static bool Initialize();
        static bool Accept(TransactionPoolVisitor* vis);
        static bool RemoveTransaction(const Hash& hash);
        static bool PutTransaction(const Handle<Transaction>& tx);
        static bool HasTransaction(const Hash& hash);
        static bool GetTransactions(std::vector<Hash>& txs);
        static Handle<Transaction> GetTransaction(const Hash& hash);

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