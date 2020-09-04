#include <glog/logging.h>
#include <unordered_map>

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
        return GetJournal()->HasData(hash);
    }

    Handle<Transaction> TransactionPool::GetTransaction(const uint256_t& hash){
        LOCK_GUARD;
        return GetJournal()->HasData(hash) ?
                GetJournal()->GetData(hash) :
                nullptr;
    }

    bool TransactionPool::RemoveTransaction(const uint256_t& hash){
        LOCK_GUARD;
        return GetJournal()->HasData(hash) ?
                GetJournal()->RemoveData(hash) :
                false;
    }

    bool TransactionPool::PutTransaction(const Handle<Transaction>& tx){
        LOCK_GUARD;
        return !GetJournal()->HasData(tx->GetHash()) ?
                GetJournal()->PutData(tx) :
                false;
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

    size_t TransactionPool::GetNumberOfTransactions(){
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
}