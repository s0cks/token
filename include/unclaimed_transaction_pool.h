#ifndef TOKEN_UNCLAIMED_TRANSACTION_POOL_H
#define TOKEN_UNCLAIMED_TRANSACTION_POOL_H

#include "unclaimed_transaction.h"
#include "memory_pool.h"

namespace Token{
    class UnclaimedTransactionPoolVisitor;
    class UnclaimedTransactionPool{
    public:
        enum State{
            kUninitialized,
            kInitializing,
            kInitialized
        };
    private:
        UnclaimedTransactionPool() = delete;

        static void SetState(State state);
    public:
        ~UnclaimedTransactionPool() = delete;

        static size_t GetSize();
        static State GetState();
        static bool Print();
        static bool Initialize();
        static bool Accept(UnclaimedTransactionPoolVisitor* vis);
        static bool RemoveUnclaimedTransaction(const uint256_t& hash);
        static bool PutUnclaimedTransaction(const Handle<UnclaimedTransaction>& utxo);
        static bool HasUnclaimedTransaction(const uint256_t& hash);
        static bool GetUnclaimedTransactions(std::vector<uint256_t>& utxos);
        static bool GetUnclaimedTransactions(const std::string& user, std::vector<uint256_t>& utxos);
        static Handle<UnclaimedTransaction> GetUnclaimedTransaction(const uint256_t& tx_hash, uint32_t tx_index);
        static Handle<UnclaimedTransaction> GetUnclaimedTransaction(const uint256_t& hash);

        static inline std::string
        GetPath(){
            return TOKEN_BLOCKCHAIN_HOME + "/unclaimed_transactions";
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

    class UnclaimedTransactionPoolVisitor{
    protected:
        UnclaimedTransactionPoolVisitor() = default;
    public:
        virtual ~UnclaimedTransactionPoolVisitor() = default;

        virtual bool VisitStart() { return true; }
        virtual bool Visit(const Handle<UnclaimedTransaction>& utxo) = 0;
        virtual bool VisitEnd() { return true; };
    };
}

#endif //TOKEN_UNCLAIMED_TRANSACTION_POOL_H