#include <dirent.h>
#include "pool.h"
#include "object.h"

namespace Token{
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
        READ_LOCK;
        UnclaimedTransactionPool* pool = GetInstance();
        if(!pool->ContainsObject(hash)){
            UNLOCK;
            return false;
        }
        UNLOCK;
        return true;
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
        LOG(INFO) << "removing unclaimed transaction: " << hash;
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

                Output output;
                if(!utxo.GetOutput(&output)){
                    LOG(ERROR) << "couldn't get output: " << utxo.GetTransaction() << "[" << utxo.GetIndex() << "]";
                    return false;
                }

                if(output.GetUser() != user) continue;
                utxos.push_back(utxo.GetHash());
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
            UnclaimedTransaction utxo;
            if(!GetUnclaimedTransaction(it, &utxo)) return false;
            if(!vis->Visit(utxo)) return false;
        }

        return vis->VisitEnd();
    }
}