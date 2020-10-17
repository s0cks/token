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

    bool TransactionPool::HasTransaction(const Hash& hash){
        LOCK_GUARD;
        return GetJournal()->HasData(hash);
    }

    Handle<Transaction> TransactionPool::GetTransaction(const Hash& hash){
        LOCK_GUARD;
        return GetJournal()->HasData(hash) ?
               GetJournal()->GetData(hash) :
               nullptr;
    }

    bool TransactionPool::RemoveTransaction(const Hash& hash){
        LOCK_GUARD;
        if(!GetJournal()->HasData(hash)) return false;
        if(!GetJournal()->RemoveData(hash)){
            LOG(WARNING) << "couldn't remove transaction " << hash << " data from pool";
            return false;
        }

        LOG(INFO) << "removed transaction " << hash << " from pool";
        return true;
    }

    bool TransactionPool::PutTransaction(const Handle<Transaction>& tx){
        LOCK_GUARD;
        Hash hash = tx->GetHash();
        if(GetJournal()->HasData(tx->GetHash())) return false;
        if(!GetJournal()->PutData(tx)){
            LOG(WARNING) << "cannot put transaction data in pool";
            return false;
        }

        LOG(INFO) << "added transaction " << hash << " to pool";
        return true;
    }

    bool TransactionPool::GetTransactions(std::vector<Hash>& txs){
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

    class TransactionPrinter : public TransactionPoolVisitor{
    public:
        bool Visit(const Handle<Transaction>& tx){
            LOG(INFO) << " - " << tx->GetHash();
            return true;
        }
    };

    bool TransactionPool::Print(){
        TransactionPrinter printer;
        return Accept(&printer);
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
}