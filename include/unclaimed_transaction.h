#ifndef TOKEN_UNCLAIMED_TRANSACTION_H
#define TOKEN_UNCLAIMED_TRANSACTION_H

#include "common.h"
#include "object.h"

namespace Token{
    class Transaction;
    class Output;
    class UnclaimedTransaction : public BinaryObject<Proto::BlockChain::UnclaimedTransaction>{
    public:
        typedef Proto::BlockChain::UnclaimedTransaction RawType;
    private:
        uint256_t hash_;
        uint32_t index_;
        std::string user_;

        UnclaimedTransaction(const uint256_t& hash, uint32_t idx, const std::string& user):
            hash_(hash),
            user_(user),
            index_(idx){}
    public:
        ~UnclaimedTransaction(){}

        uint256_t GetTransaction() const{
            return hash_;
        }

        uint32_t GetIndex() const{
            return index_;
        }

        std::string GetUser() const{
            return user_;
        }

        bool WriteToMessage(RawType& raw) const;
        std::string ToString() const;

        static Handle<UnclaimedTransaction> NewInstance(const uint256_t& hash, uint32_t index, const std::string& user);
        static Handle<UnclaimedTransaction> NewInstance(const RawType& raw);
        static Handle<UnclaimedTransaction> NewInstance(std::fstream& fd);

        static inline Handle<UnclaimedTransaction> NewInstance(const std::string& filename){
            std::fstream fd(filename, std::ios::in|std::ios::binary);
            return NewInstance(fd);
        }
    };

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

        static State GetState();
        static void Initialize();
        static void RemoveUnclaimedTransaction(const uint256_t& hash);
        static void PutUnclaimedTransaction(UnclaimedTransaction* utxo);
        static bool HasUnclaimedTransaction(const uint256_t& hash);
        static bool Accept(UnclaimedTransactionPoolVisitor* vis);
        static bool GetUnclaimedTransactions(std::vector<uint256_t>& utxos);
        static bool GetUnclaimedTransactions(const std::string& user, std::vector<uint256_t>& utxos);
        static UnclaimedTransaction* GetUnclaimedTransaction(const uint256_t& tx_hash, uint32_t tx_index);
        static UnclaimedTransaction* GetUnclaimedTransaction(const uint256_t& hash);

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

#ifdef TOKEN_DEBUG
        static void PrintUnclaimedTransactions();
#endif//TOKEN_DEBUG
    };

    class UnclaimedTransactionPoolVisitor{
    public:
        virtual ~UnclaimedTransactionPoolVisitor() = default;

        virtual bool VisitStart() { return true; }
        virtual bool Visit(UnclaimedTransaction* utxo) = 0;
        virtual bool VisitEnd() { return true; };
    };
}

#endif //TOKEN_UNCLAIMED_TRANSACTION_H