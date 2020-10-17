#include "unclaimed_transaction_pool.h"
#include "journal.h"

namespace Token{
    static std::recursive_mutex mutex_;
    static UnclaimedTransactionPool::State state_ = UnclaimedTransactionPool::kUninitialized;

#define LOCK_GUARD std::lock_guard<std::recursive_mutex> guard(mutex_);

    static inline ObjectPoolJournal<UnclaimedTransaction>*
    GetJournal(){
        static ObjectPoolJournal<UnclaimedTransaction> journal(UnclaimedTransactionPool::GetPath());
        return &journal;
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
        if(!GetJournal()->IsInitialized()){
            LOG(WARNING) << "couldn't initialize unclaimed transaction pool object cache: " << GetPath();
            SetState(UnclaimedTransactionPool::kUninitialized);
            return false;
        }

        SetState(kInitialized);
        LOG(INFO) << "initialized the unclaimed transaction pool in: " << GetPath();
        return true;
    }

    bool UnclaimedTransactionPool::HasUnclaimedTransaction(const Hash& hash){
        LOCK_GUARD;
        return GetJournal()->HasData(hash);
    }

    Handle<UnclaimedTransaction> UnclaimedTransactionPool::GetUnclaimedTransaction(const Hash& hash){
        LOCK_GUARD;
        return GetJournal()->GetData(hash);
    }

    bool UnclaimedTransactionPool::RemoveUnclaimedTransaction(const Hash& hash){
        LOCK_GUARD;
        return GetJournal()->RemoveData(hash);
    }

    bool UnclaimedTransactionPool::PutUnclaimedTransaction(const Handle<UnclaimedTransaction>& utxo){
        LOCK_GUARD;
        return GetJournal()->PutData(utxo);
    }

    size_t UnclaimedTransactionPool::GetSize(){
        LOCK_GUARD;
        return GetJournal()->GetNumberOfObjects();
    }

    bool UnclaimedTransactionPool::GetUnclaimedTransactions(std::vector<Hash>& utxos){
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

    bool UnclaimedTransactionPool::GetUnclaimedTransactions(const std::string& user, std::vector<Hash>& utxos){
        LOCK_GUARD;
        DIR* dir;
        struct dirent* ent;
        if((dir = opendir(GetPath().c_str())) != NULL){
            while((ent = readdir(dir)) != NULL){
                std::string name(ent->d_name);
                std::string filename = (GetPath() + "/" + name);
                if(!EndsWith(filename, ".dat")) continue;
                Handle<UnclaimedTransaction> utxo = UnclaimedTransaction::NewInstance(filename);

                if(utxo->GetUser() != user) continue;
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

    Handle<UnclaimedTransaction> UnclaimedTransactionPool::GetUnclaimedTransaction(const Hash &tx_hash, uint32_t tx_index){
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

    class UnclaimedTransactionPoolPrinter : public UnclaimedTransactionPoolVisitor{
    public:
        UnclaimedTransactionPoolPrinter(): UnclaimedTransactionPoolVisitor(){}
        ~UnclaimedTransactionPoolPrinter() = default;

        bool Visit(const Handle<UnclaimedTransaction>& utxo){
            LOG(INFO) << utxo->GetTransaction() << "[" << utxo->GetIndex() << "] := " << utxo->GetHash();
            return true;
        }
    };

    bool UnclaimedTransactionPool::Print(){
        UnclaimedTransactionPoolPrinter printer;
        return Accept(&printer);
    }
}