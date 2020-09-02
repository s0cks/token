#include "unclaimed_transaction_pool.h"

namespace Token{
    static std::recursive_mutex mutex_;
    static UnclaimedTransactionPool::State state_ = UnclaimedTransactionPool::kUninitialized;

    static ObjectCache<UnclaimedTransaction>* cache_ = nullptr;

#define LOCK_GUARD std::lock_guard<std::recursive_mutex> guard(mutex_);

    ObjectCache<UnclaimedTransaction>* UnclaimedTransactionPool::GetCache(){
        LOCK_GUARD;
        return cache_;
    }

    void UnclaimedTransactionPool::SetState(UnclaimedTransactionPool::State state){
        LOCK_GUARD;
        state_ = state;
    }

    UnclaimedTransactionPool::State UnclaimedTransactionPool::GetState(){
        LOCK_GUARD;
        return state_;
    }

    bool UnclaimedTransactionPool::Initialize(){
        if(IsInitialized()){
            LOG(WARNING) << "cannot re-initialize the unclaimed transaction pool in: " << GetPath();
            return false;
        }

        LOG(INFO) << "initializing the unclaimed transaction pool....";
        SetState(kInitializing);
        if(!(cache_ = new ObjectCache<UnclaimedTransaction>(GetPath())) && !cache_->IsInitialized()){
            LOG(WARNING) << "couldn't initialize unclaimed transaction pool object cache: " << GetPath();
            SetState(UnclaimedTransactionPool::kUninitialized);
            return false;
        }

        SetState(kInitialized);
        LOG(INFO) << "initialized the unclaimed transaction pool in: " << GetPath();
        return true;
    }

    bool UnclaimedTransactionPool::HasUnclaimedTransaction(const uint256_t& hash){
        LOCK_GUARD;
        return GetCache()->HasData(hash);
    }

    Handle<UnclaimedTransaction> UnclaimedTransactionPool::GetUnclaimedTransaction(const uint256_t& hash){
        LOCK_GUARD;
        return GetCache()->GetData(hash);
    }

    bool UnclaimedTransactionPool::RemoveUnclaimedTransaction(const uint256_t& hash){
        LOCK_GUARD;
        return GetCache()->RemoveData(hash);
    }

    bool UnclaimedTransactionPool::PutUnclaimedTransaction(const Handle<UnclaimedTransaction>& utxo){
        LOCK_GUARD;
        return GetCache()->PutData(utxo);
    }

    bool UnclaimedTransactionPool::GetUnclaimedTransactions(std::vector<uint256_t>& utxos){
        LOCK_GUARD;
        DIR* dir;
        struct dirent* ent;
        if((dir = opendir(GetPath().c_str())) != NULL){
            while((ent = readdir(dir)) != NULL){
                std::string name(ent->d_name);
                std::string filename = (GetPath() + "/" + name);
                if(!EndsWith(filename, ".dat")) continue;

                Handle<UnclaimedTransaction> utxo = UnclaimedTransaction::NewInstance(filename);
                utxos.push_back(utxo->GetHash());
            }
            closedir(dir);
            return true;
        }

        return false;
    }

    bool UnclaimedTransactionPool::GetUnclaimedTransactions(const std::string& user, std::vector<uint256_t>& utxos){
        LOCK_GUARD;
        DIR* dir;
        struct dirent* ent;
        if((dir = opendir(GetPath().c_str())) != NULL){
            while((ent = readdir(dir)) != NULL){
                std::string name(ent->d_name);
                std::string filename = (GetPath() + "/" + name);
                if(!EndsWith(filename, ".dat")) continue;
                Handle<UnclaimedTransaction> utxo = UnclaimedTransaction::NewInstance(filename);

                LOG(INFO) << "user: " << utxo->GetUser();
                if(utxo->GetUser() != user){
                    LOG(WARNING) << "skipping: " << utxo;
                    continue;
                }
                utxos.push_back(utxo->GetHash());
            }
            closedir(dir);
            return true;
        }
        return false;
    }

    bool UnclaimedTransactionPool::Accept(UnclaimedTransactionPoolVisitor* vis){
        if(!vis->VisitStart()) return false;

        LOCK_GUARD;
        DIR* dir;
        struct dirent* ent;
        if((dir = opendir(GetPath().c_str())) != NULL){
            while((ent = readdir(dir)) != NULL){
                std::string name(ent->d_name);
                std::string filename = (GetPath() + "/" + name);
                if(!EndsWith(filename, ".dat")) continue;
                Handle<UnclaimedTransaction> utxo = UnclaimedTransaction::NewInstance(filename);
                if(!vis->Visit(utxo)) break;
            }
            closedir(dir);
        }

        return vis->VisitEnd();
    }

    Handle<UnclaimedTransaction> UnclaimedTransactionPool::GetUnclaimedTransaction(const uint256_t &tx_hash, uint32_t tx_index){
        LOCK_GUARD;
        DIR* dir;
        struct dirent* ent;
        if((dir = opendir(GetPath().c_str())) != NULL){
            while((ent = readdir(dir)) != NULL){
                std::string name(ent->d_name);
                std::string filename = (GetPath() + "/" + name);
                if(!EndsWith(filename, ".dat")) continue;

                Handle<UnclaimedTransaction> utxo = UnclaimedTransaction::NewInstance(filename);
                if(utxo->GetTransaction() == tx_hash && utxo->GetIndex() == tx_index) return utxo;
            }
            closedir(dir);
        }
        LOG(WARNING) << "couldn't find unclaimed transaction: " << tx_hash << "[" << tx_index << "]";
        return nullptr;
    }

    size_t UnclaimedTransactionPool::GetNumberOfUnclaimedTransactions(){
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

    class UnclaimedTransactionPoolPrinter : public UnclaimedTransactionPoolVisitor{
    public:
        UnclaimedTransactionPoolPrinter(): UnclaimedTransactionPoolVisitor(){}
        ~UnclaimedTransactionPoolPrinter() = default;

        bool Visit(const Handle<UnclaimedTransaction>& utxo){
            LOG(INFO) << utxo->GetTransaction() << "[" << utxo->GetIndex() << "] := " << utxo->GetHash();
            return true;
        }
    };

    bool UnclaimedTransactionPool::PrintUnclaimedTransactions(){
        UnclaimedTransactionPoolPrinter printer;
        return Accept(&printer);
    }
}