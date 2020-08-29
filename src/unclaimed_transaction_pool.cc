#include "crash_report.h"
#include "unclaimed_transaction_pool.h"
#include "unclaimed_transaction_pool_index.h"

namespace Token{
    static std::recursive_mutex mutex_;
    static UnclaimedTransactionPool::State state_ = UnclaimedTransactionPool::kUninitialized;

    static inline std::string
    GetDataDirectory(){
        return FLAGS_path + "/utxos";
    }

#define LOCK_GUARD std::lock_guard<std::recursive_mutex> guard(mutex_);

    void UnclaimedTransactionPool::SetState(UnclaimedTransactionPool::State state){
        LOCK_GUARD;
        state_ = state;
    }

    UnclaimedTransactionPool::State UnclaimedTransactionPool::GetState(){
        LOCK_GUARD;
        return state_;
    }

    void UnclaimedTransactionPool::Initialize(){
        if(!IsUninitialized()){
            std::stringstream ss;
            ss << "Cannot re-initialize unclaimed transaction pool";
            CrashReport::GenerateAndExit(ss);
        }

        SetState(kInitializing);
        UnclaimedTransactionPoolIndex::Initialize();
        SetState(kInitialized);
#ifdef TOKEN_DEBUG
        LOG(INFO) << "initialized the unclaimed transaction pool";
#endif//TOKEN_DEBUG
    }

    bool UnclaimedTransactionPool::HasUnclaimedTransaction(const uint256_t& hash){
        LOCK_GUARD;
        return UnclaimedTransactionPoolIndex::HasData(hash);
    }

    Handle<UnclaimedTransaction> UnclaimedTransactionPool::GetUnclaimedTransaction(const uint256_t& hash){
        LOCK_GUARD;
        if(!UnclaimedTransactionPoolIndex::HasData(hash)) return Handle<UnclaimedTransaction>(); //null
        return UnclaimedTransactionPoolIndex::GetData(hash);
    }

    void UnclaimedTransactionPool::RemoveUnclaimedTransaction(const uint256_t& hash){
        LOCK_GUARD;
        if(!HasUnclaimedTransaction(hash)) return;
        UnclaimedTransactionPoolIndex::RemoveData(hash);
    }

    void UnclaimedTransactionPool::PutUnclaimedTransaction(const Handle<UnclaimedTransaction>& utxo){
        LOCK_GUARD;
        UnclaimedTransactionPoolIndex::PutData(utxo);
    }

    bool UnclaimedTransactionPool::GetUnclaimedTransactions(std::vector<uint256_t>& utxos){
        LOCK_GUARD;
        DIR* dir;
        struct dirent* ent;
        if((dir = opendir(GetDataDirectory().c_str())) != NULL){
            while((ent = readdir(dir)) != NULL){
                std::string name(ent->d_name);
                std::string filename = (GetDataDirectory() + "/" + name);
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
        if((dir = opendir(GetDataDirectory().c_str())) != NULL){
            while((ent = readdir(dir)) != NULL){
                std::string name(ent->d_name);
                std::string filename = (GetDataDirectory() + "/" + name);
                if(!EndsWith(filename, ".dat")) continue;
                Handle<UnclaimedTransaction> utxo = UnclaimedTransaction::NewInstance(filename);
                if(utxo->GetUser().Get() != user) continue;
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
        if((dir = opendir(GetDataDirectory().c_str())) != NULL){
            while((ent = readdir(dir)) != NULL){
                std::string name(ent->d_name);
                std::string filename = (GetDataDirectory() + "/" + name);
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
        if((dir = opendir(GetDataDirectory().c_str())) != NULL){
            while((ent = readdir(dir)) != NULL){
                std::string name(ent->d_name);
                std::string filename = (GetDataDirectory() + "/" + name);
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
        if((dir = opendir(GetDataDirectory().c_str())) != NULL){
            while((ent = readdir(dir)) != NULL){
                std::string name(ent->d_name);
                std::string filename = (GetDataDirectory() + "/" + name);
                if(!EndsWith(filename, ".dat")) continue;
                size++;
            }
            closedir(dir);
        }
        return size;
    }

#ifdef TOKEN_DEBUG
    class UnclaimedTransactionPoolPrinter : public UnclaimedTransactionPoolVisitor{
    public:
        UnclaimedTransactionPoolPrinter(): UnclaimedTransactionPoolVisitor(){}
        ~UnclaimedTransactionPoolPrinter() = default;

        bool Visit(const Handle<UnclaimedTransaction>& utxo){
            LOG(INFO) << utxo->GetTransaction() << "[" << utxo->GetIndex() << "] := " << utxo->GetHash();
            return true;
        }
    };

    void UnclaimedTransactionPool::PrintUnclaimedTransactions(){
        UnclaimedTransactionPoolPrinter printer;
        if(!Accept(&printer)) CrashReport::GenerateAndExit("Couldn't print unclaimed transactions in pool");
    }
#endif//TOKEN_DEBUG
}