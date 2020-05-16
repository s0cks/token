#include <dirent.h>
#include "pool.h"
#include "object.h"

namespace Token{
#define READ_LOCK pthread_rwlock_tryrdlock(&pool->rwlock_)
#define WRITE_LOCK pthread_rwlock_trywrlock(&pool->rwlock_)
#define UNLOCK pthread_rwlock_unlock(&pool->rwlock_)

    TransactionPool* TransactionPool::GetInstance(){
        static TransactionPool instance;
        return &instance;
    }

    bool TransactionPool::Initialize(){
        TransactionPool* pool = GetInstance();
        if(!FileExists(pool->GetRoot())){
            if(!CreateDirectory(pool->GetRoot())) return false;
        }
        return pool->InitializeIndex();
    }

    bool TransactionPool::HasTransaction(const uint256_t& hash){
        TransactionPool* pool = GetInstance();
        READ_LOCK;
        if(!pool->ContainsObject(hash)){
            UNLOCK;
            return false;
        }
        UNLOCK;
        return true;
    }

    bool TransactionPool::GetTransaction(const uint256_t& hash, Transaction* result){
        TransactionPool* pool = GetInstance();
        READ_LOCK;
        if(!pool->LoadObject(hash, result)){
            UNLOCK;
            return false;
        }
        UNLOCK;
        return true;
    }

    bool TransactionPool::GetTransactions(std::vector<Transaction>& txs){
        TransactionPool* pool = GetInstance();
        READ_LOCK;
        DIR* dir;
        struct dirent* ent;
        if((dir = opendir(pool->GetRoot().c_str())) != NULL){
            while((ent = readdir(dir)) != NULL){
                std::string name(ent->d_name);
                std::string filename = (pool->GetRoot() + "/" + name);
                if(!EndsWith(filename, ".dat")) continue;
                Transaction tx;
                if(!pool->LoadRawObject(filename, &tx)){
                    LOG(ERROR) << "couldn't load transaction from: " << filename;
                    return false;
                }
                txs.push_back(tx);
            }
            closedir(dir);
            UNLOCK;
            return true;
        }
        UNLOCK;
        return false;
    }

    bool TransactionPool::RemoveTransaction(const uint256_t& hash){
        TransactionPool* pool = GetInstance();
        WRITE_LOCK;
        if(!pool->DeleteObject(hash)){
            UNLOCK;
            return false;
        }
        UNLOCK;
        return true;
    }

    bool TransactionPool::PutTransaction(Transaction* tx){
        TransactionPool* pool = GetInstance();
        WRITE_LOCK;
        uint256_t hash = tx->GetHash();
        if(!pool->SaveObject(hash, tx)){
            UNLOCK;
            return false;
        }
        UNLOCK;
        return true;
    }

    uint64_t TransactionPool::GetSize(){
        TransactionPool* pool = GetInstance();
        READ_LOCK;
        uint64_t size = 0;
        DIR* dir;
        if((dir = opendir(pool->GetRoot().c_str())) != nullptr){
            struct dirent* ent;
            while((ent = readdir(dir))){
                std::string name(ent->d_name);
                if(EndsWith(name, ".dat")) size++;
            }
        }
        UNLOCK;
        return size;
    }
}