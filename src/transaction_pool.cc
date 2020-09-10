#include <glog/logging.h>
#include <unordered_map>

#include "cache.h"
#include "journal.h"
#include "crash_report.h"
#include "transaction_pool.h"

namespace Token{
    static std::recursive_mutex mutex_;
    static TransactionPool::State state_;

#define LOCK_GUARD std::lock_guard<std::recursive_mutex> guard(mutex_);

    static inline ObjectPoolJournal<Transaction>*
    GetJournal(){
        static ObjectPoolJournal<Transaction> journal(TransactionPool::GetPath());
        return &journal;
    }

    static inline MemoryPoolCache*
    GetCache(){
        static MemoryPool* pool = Allocator::GetTransactionPoolMemory();
        static MemoryPoolCache cache(pool, pool->GetSize() / sizeof(Transaction));
        return &cache;
    }

    TransactionPool::State TransactionPool::GetState(){
        LOCK_GUARD;
        return state_;
    }

    void TransactionPool::SetState(TransactionPool::State state){
        LOCK_GUARD;
        state_ = state;
    }

    bool TransactionPool::Accept(TransactionPoolVisitor* vis){
        LOCK_GUARD;
        DIR* dir;
        struct dirent* ent;
        if((dir = opendir(GetPath().c_str())) != NULL){
            while((ent = readdir(dir)) != NULL){
                std::string name(ent->d_name);
                std::string filename = (GetPath() + "/" + name);
                if(!EndsWith(filename, ".dat")) continue;

                Handle<Transaction> tx = Transaction::NewInstance(filename);
                if(!vis->Visit(tx)) return false;
            }
            closedir(dir);
        }
        return true;
    }

    bool TransactionPool::Initialize(){
        if(IsInitialized()){
            LOG(WARNING) << "couldn't re-initialize the transaction pool";
            return false;
        }

        LOG(INFO) << "initializing the transaction pool....";
        SetState(TransactionPool::kInitializing);
        if(!GetJournal()->IsInitialized()){
            LOG(WARNING) << "couldn't initialize the transaction pool object cache: " << GetPath();
            SetState(TransactionPool::kUninitialized);
            return false;
        }

        SetState(TransactionPool::kInitialized);
        LOG(INFO) << "initialized the transaction pool";
        return true;
    }

    bool TransactionPool::HasTransaction(const uint256_t& hash){
        LOCK_GUARD;
        return GetCache()->ContainsItem(hash)
            || GetJournal()->HasData(hash);
    }

    Handle<Transaction> TransactionPool::GetTransaction(const uint256_t& hash){
        LOCK_GUARD;
        if(GetCache()->ContainsItem(hash)){
            return (Transaction*)GetCache()->GetItem(hash);
        }

        if(GetJournal()->HasData(hash)){
            Handle<Transaction> utxo = GetJournal()->GetData(hash);
            if(!GetCache()->PutItem(utxo)) LOG(WARNING) << "cannot put transaction " << hash << " into pool cache";
            return utxo;
        }

        return nullptr;
    }

    bool TransactionPool::RemoveTransaction(const uint256_t& hash){
        LOCK_GUARD;
        if(GetCache()->ContainsItem(hash))
            GetCache()->RemoveItem(hash);
        return GetJournal()->RemoveData(hash);
    }

    bool TransactionPool::PutTransaction(const Handle<Transaction>& tx){
        LOCK_GUARD;
        uint256_t hash = tx->GetHash();
        if(GetCache()->ContainsItem(hash) || GetJournal()->HasData(hash))
            return false;
        if(!GetCache()->PutItem(tx)) {
            LOG(WARNING) << "couldn't put transaction " << hash << " in pool cache";
            return false;
        }

        if(!GetJournal()->PutData(tx)){
            LOG(WARNING) << "couldn't write transaction " << hash << " to disk";
            if(!GetCache()->RemoveItem(hash)){
                LOG(WARNING) << "couldn't remove transaction " << hash << " from pool cache";
                return false;
            }
            return false;
        }

        return true;
    }

    bool TransactionPool::GetTransactions(std::vector<uint256_t>& txs){
        LOCK_GUARD;
        DIR* dir;
        struct dirent* ent;
        if((dir = opendir(GetPath().c_str())) != NULL){
            while((ent = readdir(dir)) != NULL){
                std::string name(ent->d_name);
                std::string filename = (GetPath() + "/" + name);
                if(!EndsWith(filename, ".dat")) continue;

                Handle<Transaction> tx = Transaction::NewInstance(filename);
                txs.push_back(tx->GetHash());
            }
            closedir(dir);
            return true;
        }
        return false;
    }

    void TransactionPool::Print(bool cache_only){
        ObjectPrinter printer;
        if(cache_only){
            LOG(INFO) << "Transaction Pool (Cache):";
            if(!GetCache()->Accept(&printer)){
                LOG(WARNING) << "couldn't print transaction pool cache";
                return;
            }
        } else{
            //TODO: implement TransactionPool::PrintPool(cache_only=false)
            return;
        }
    }

    size_t TransactionPool::GetSize(){
        size_t size = 0;

        LOCK_GUARD;
        DIR* dir;
        struct dirent* ent;
        if((dir = opendir(GetPath().c_str())) != NULL){
            while((ent = readdir(dir)) != NULL){
                std::string name(ent->d_name);
                std::string filename = (GetPath() + "/" + name);
                if(!EndsWith(filename, ".dat")) continue;
                size++;
            }
            closedir(dir);
        }
        return size;
    }

    size_t TransactionPool::GetCacheSize(){
        return GetCache()->GetSize();
    }

    size_t TransactionPool::GetMaxCacheSize(){
        return GetCache()->GetMaxSize();
    }
}