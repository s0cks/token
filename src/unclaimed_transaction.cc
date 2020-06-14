#include "block_chain.h"

namespace Token{
//######################################################################################################################
//                                          Unclaimed Transaction
//######################################################################################################################
    UnclaimedTransaction* UnclaimedTransaction::NewInstance(const uint256_t &hash, uint32_t index){
        UnclaimedTransaction* instance = (UnclaimedTransaction*)Allocator::Allocate(sizeof(UnclaimedTransaction));
        new (instance)UnclaimedTransaction(hash, index);
        return instance;
    }

    UnclaimedTransaction* UnclaimedTransaction::NewInstance(const UnclaimedTransaction::RawType& raw){
        return NewInstance(HashFromHexString(raw.tx_hash()), raw.tx_index());
    }

    std::string UnclaimedTransaction::ToString() const{
        std::stringstream stream;
        stream << "UnclaimedTransaction(" << hash_ << "[" << index_ << "])";
        return stream.str();
    }

    bool UnclaimedTransaction::Encode(Token::UnclaimedTransaction::RawType &raw) const{
        raw.set_tx_hash(HexString(hash_));
        raw.set_tx_index(index_);
        return true;
    }

//######################################################################################################################
//                                          Unclaimed Transaction Pool
//######################################################################################################################
    static pthread_rwlock_t kPoolLock = PTHREAD_RWLOCK_INITIALIZER;
#define READ_LOCK pthread_rwlock_tryrdlock(&kPoolLock)
#define WRITE_LOCK pthread_rwlock_trywrlock(&kPoolLock)
#define UNLOCK pthread_rwlock_unlock(&kPoolLock)

    UnclaimedTransactionPool* UnclaimedTransactionPool::GetInstance(){
        static UnclaimedTransactionPool instance;
        return &instance;
    }

    bool UnclaimedTransactionPool::Initialize(){
        UnclaimedTransactionPool* pool = GetInstance();
        if(!FileExists(pool->GetRoot())){
            if(!CreateDirectory(pool->GetRoot())) return false;
        }
        return pool->InitializeIndex();
    }

    bool UnclaimedTransactionPool::HasUnclaimedTransaction(const uint256_t& hash){
        UnclaimedTransactionPool* pool = GetInstance();
        READ_LOCK;
        bool found = pool->ContainsObject(hash);
        UNLOCK;
        return found;
    }

    UnclaimedTransaction* UnclaimedTransactionPool::GetUnclaimedTransaction(const uint256_t& hash){
        UnclaimedTransactionPool* pool = GetInstance();
        READ_LOCK;
        UnclaimedTransaction* utxo = pool->LoadObject(hash);
        UNLOCK;
        return utxo;
    }

    bool UnclaimedTransactionPool::RemoveUnclaimedTransaction(const uint256_t& hash){
        WRITE_LOCK;
        UnclaimedTransactionPool* pool = GetInstance();
        if(!pool->DeleteObject(hash)){
            UNLOCK;
            return false;
        }
        UNLOCK;
        return true;
    }

    bool UnclaimedTransactionPool::PutUnclaimedTransaction(UnclaimedTransaction* utxo){
        WRITE_LOCK;
        UnclaimedTransactionPool* pool = GetInstance();
        uint256_t hash = utxo->GetHash();
        if(!pool->SaveObject(hash, utxo)){
            UNLOCK;
            return false;
        }
        UNLOCK;
        return true;
    }

    bool UnclaimedTransactionPool::GetUnclaimedTransactions(std::vector<uint256_t>& utxos){
        READ_LOCK;
        UnclaimedTransactionPool* pool = GetInstance();
        DIR* dir;
        struct dirent* ent;
        if((dir = opendir(pool->GetRoot().c_str())) != NULL){
            while((ent = readdir(dir)) != NULL){
                std::string name(ent->d_name);
                std::string filename = (pool->GetRoot() + "/" + name);
                if(!EndsWith(filename, ".dat")) continue;
                UnclaimedTransaction* utxo;
                if(!(utxo = pool->LoadRawObject(filename))){
                    LOG(ERROR) << "couldn't load unclaimed transaction from: " << filename;
                    return false;
                }

                LOG(INFO) << "utxo: " << utxo->GetHash();
                utxos.push_back(utxo->GetHash());
            }
            closedir(dir);
            UNLOCK;
            return true;
        }
        UNLOCK;
        return false;
    }

    bool UnclaimedTransactionPool::GetUnclaimedTransactions(const std::string& user, std::vector<uint256_t>& utxos){
        READ_LOCK;
        UnclaimedTransactionPool* pool = GetInstance();
        DIR* dir;
        struct dirent* ent;
        if((dir = opendir(pool->GetRoot().c_str())) != NULL){
            while((ent = readdir(dir)) != NULL){
                std::string name(ent->d_name);
                std::string filename = (pool->GetRoot() + "/" + name);
                if(!EndsWith(filename, ".dat")) continue;
                UnclaimedTransaction* utxo;
                if(!(utxo = pool->LoadRawObject(filename))){
                    LOG(ERROR) << "couldn't load unclaimed transaction from: " << filename;
                    return false;
                }

                // if(utxo.GetUser() != user) continue;
                utxos.push_back(utxo->GetHash());
            }
            closedir(dir);
            UNLOCK;
            return true;
        }
        UNLOCK;
        return false;
    }

    bool UnclaimedTransactionPool::Accept(UnclaimedTransactionPoolVisitor* vis){
        if(!vis->VisitStart()) return false;

        std::vector<uint256_t> utxos;
        if(!GetUnclaimedTransactions(utxos)) return false;

        for(auto& it : utxos){
            UnclaimedTransaction* utxo;
            if(!(utxo = GetUnclaimedTransaction(it))) return false;
            if(!vis->Visit(utxo)) return false;
        }

        return vis->VisitEnd();
    }
}
