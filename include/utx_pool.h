#ifndef TOKEN_UTX_POOL_H
#define TOKEN_UTX_POOL_H

#include "common.h"
#include "block.h"
#include <map>

namespace Token{
    class BlockChain;
    class UnclaimedTransactionPool;

    class UnclaimedTransaction{
    private:
        UnclaimedTransactionPool* owner_;
        Transaction* transaction_;
        Messages::UnclaimedTransaction* raw_;

        inline Messages::UnclaimedTransaction*
        GetRaw() const{
            return raw_;
        }
    public:
        UnclaimedTransaction(UnclaimedTransactionPool* owner, Transaction* tx, int index):
            owner_(owner),
            transaction_(tx),
            raw_(new Messages::UnclaimedTransaction()){
            GetRaw()->set_index(index);
            GetRaw()->set_hash(tx->GetHash());
        }
        UnclaimedTransaction(UnclaimedTransactionPool* owner, std::string tx_hash, int index):
            owner_(owner),
            transaction_(nullptr),
            raw_(new Messages::UnclaimedTransaction()){
            GetRaw()->set_hash(tx_hash);
            GetRaw()->set_index(index);
        }

        UnclaimedTransactionPool* GetOwner() const{
            return owner_;
        }

        int GetIndex() const{
            return GetRaw()->index();
        }

        std::string GetHash() const{
            return GetRaw()->hash();
        }

        Transaction* GetTransaction() const{
            return transaction_;
        }

        BlockChain* GetChain() const;
    };

    class UnclaimedTransactionPool{
    private:
        BlockChain* chain_;
        std::map<std::string, UnclaimedTransaction*> utxs_;
    public:
        UnclaimedTransactionPool(BlockChain* chain):
            utxs_(),
            chain_(chain){}
        UnclaimedTransactionPool(const UnclaimedTransactionPool& other):
            utxs_(),
            chain_(other.GetChain()){
            utxs_.insert(other.utxs_.begin(), other.utxs_.end());
        }
        ~UnclaimedTransactionPool(){}

        bool Contains(std::string hash) const{
            return utxs_.find(hash) != utxs_.end();
        }

        bool Contains(UnclaimedTransaction* utx) const{
            return Contains(utx->GetHash());
        }

        UnclaimedTransaction* Get(std::string hash) const{
            if(!Contains(hash)) return nullptr;
            return utxs_.find(hash)->second;
        }

        void Remove(std::string hash){
            utxs_.erase(hash);
        }

        void Register(Transaction* tx, int index){
            UnclaimedTransaction* utx = new UnclaimedTransaction(this, tx, index);
            utxs_.insert(std::make_pair(tx->GetHash(), utx));
        }

        BlockChain* GetChain() const{
            return chain_;
        }
    };
}

#endif //TOKEN_UTX_POOL_H