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
        //TODO: implement
        if(!HasUnclaimedTransaction(hash)) return;
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
                utxos.push_back(utxo->GetSHA256Hash());
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
                if(utxo->GetUser() != user) continue;
                utxos.push_back(utxo->GetSHA256Hash());
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
        std::vector<uint256_t> utxos;
        if(!GetUnclaimedTransactions(utxos)) {
            std::stringstream ss;
            ss << "Couldn't get all unclaimed transactions from pool";
            CrashReport::GenerateAndExit(ss);
        }

        for(auto& it : utxos){
            //TODO: refactor
            UnclaimedTransaction* utxo;
            if(!(utxo = GetUnclaimedTransaction(it))){
                std::stringstream ss;
                ss << "Couldn't get unclaimed transaction " << it << " from pool";
                CrashReport::GenerateAndExit(ss);
            }
            if(utxo->GetTransaction() == tx_hash && utxo->GetIndex() == tx_index) return utxo;
        }

        LOG(WARNING) << "couldn't find unclaimed transaction: " << tx_hash << "[" << tx_index << "]";
        return nullptr;
    }

#ifdef TOKEN_DEBUG
    class UnclaimedTransactionPoolPrinter : public UnclaimedTransactionPoolVisitor{
    public:
        UnclaimedTransactionPoolPrinter(): UnclaimedTransactionPoolVisitor(){}
        ~UnclaimedTransactionPoolPrinter() = default;

        bool Visit(const Handle<UnclaimedTransaction>& utxo){
            LOG(INFO) << utxo->GetTransaction() << "[" << utxo->GetIndex() << "] := " << utxo->GetSHA256Hash();
            return true;
        }
    };

    void UnclaimedTransactionPool::PrintUnclaimedTransactions(){
        UnclaimedTransactionPoolPrinter printer;
        if(!Accept(&printer)) CrashReport::GenerateAndExit("Couldn't print unclaimed transactions in pool");
    }
#endif//TOKEN_DEBUG
}