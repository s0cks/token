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

    static inline Cache<Transaction, 1>*
    GetCache(){
        static Cache<Transaction, 1> cache;
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
        if(!GetJournal()->HasData(hash)) return false;

        if(GetCache()->Contains(hash)){
            if(!GetCache()->Remove(hash)){
                LOG(WARNING) << "couldn't remove transaction " << hash << " from pool";
                return false;
            }
        }

        if(!GetJournal()->RemoveData(hash)){
            LOG(WARNING) << "couldn't remove transaction " << hash << " data from pool";
            return false;
        }

        LOG(INFO) << "removed transaction " << hash << " from pool";
        return true;
    }

    bool TransactionPool::PutTransaction(const Handle<Transaction>& tx){
        LOCK_GUARD;
        uint256_t hash = tx->GetHash();
        if(GetJournal()->HasData(tx->GetHash())) return false;

        if(!GetCache()->Put(hash, tx)){
            LOG(WARNING) << "cannot put transaction " << hash << " in pool";
            return false;
        }

        if(!GetJournal()->PutData(tx)){
            LOG(WARNING) << "cannot put transaction data in pool";
            return false;
        }

        LOG(INFO) << "added transaction " << hash << " to pool";
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

    class ObjectPrinter : public ObjectPointerVisitor{
    public:
        ObjectPrinter():
            ObjectPointerVisitor(){}
        ~ObjectPrinter() = default;

        bool Visit(RawObject* obj){
            LOG(INFO) << obj->ToString();
            return true;
        }
    };

    void TransactionPool::PrintPool(bool cache_only){
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