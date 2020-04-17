#include <dirent.h>
#include <glog/logging.h>
#include "common.h"
#include "unclaimed_transaction.h"
#include "block_chain.h"

namespace Token{
    bool UnclaimedTransaction::GetBytes(CryptoPP::SecByteBlock& bytes) const{
        Proto::BlockChain::UnclaimedTransaction raw;
        raw << (*this);
        bytes.resize(raw.ByteSizeLong());
        return raw.SerializeToArray(bytes.data(), bytes.size());
    }

    static pthread_rwlock_t kPoolLock = PTHREAD_RWLOCK_INITIALIZER;
#define READ_LOCK pthread_rwlock_tryrdlock(&kPoolLock);
#define WRITE_LOCK pthread_rwlock_trywrlock(&kPoolLock);
#define UNLOCK pthread_rwlock_unlock(&kPoolLock);

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

    bool UnclaimedTransactionPool::GetUnclaimedTransaction(const uint256_t& hash, UnclaimedTransaction* result){
        READ_LOCK;
        UnclaimedTransactionPool* pool = GetInstance();
        if(!pool->LoadObject(hash, result)){
            UNLOCK;
            (*result) = UnclaimedTransaction();
            return false;
        }

        UNLOCK;
        return true;
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
                UnclaimedTransaction utxo;
                if(!pool->LoadRawObject(filename, &utxo)){
                    LOG(ERROR) << "couldn't load unclaimed transaction from: " << filename;
                    return false;
                }
                utxos.push_back(utxo.GetHash());
            }
            closedir(dir);
            UNLOCK;
            LOG(INFO) << "found " << utxos.size() << " utxos for all users";
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
                UnclaimedTransaction utxo;
                if(!pool->LoadRawObject(filename, &utxo)){
                    LOG(ERROR) << "couldn't load unclaimed transaction from: " << filename;
                    return false;
                }
                LOG(INFO) << utxo.GetUser();
                if(utxo.GetUser() != user) continue;
                utxos.push_back(utxo.GetHash());
            }
            closedir(dir);
            UNLOCK;
            LOG(INFO) << "found " << utxos.size() << " utxos for " << user;
            return true;
        }
        UNLOCK;
        return false;
    }
}