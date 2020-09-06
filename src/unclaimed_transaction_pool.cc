#include "unclaimed_transaction_pool.h"
#include "journal.h"

namespace Token{
    static std::mutex mutex_;
    static UnclaimedTransactionPool::State state_ = UnclaimedTransactionPool::kUninitialized;

#define LOCK_GUARD std::lock_guard<std::mutex> guard(mutex_);

    static inline ObjectPoolJournal<UnclaimedTransaction>*
    GetJournal(){
        static ObjectPoolJournal<UnclaimedTransaction> journal(UnclaimedTransactionPool::GetPath());
        return &journal;
    }

    static inline MemoryPoolCache*
    GetCache(){
        static MemoryPool* mem_pool = Allocator::GetUnclaimedTransactionPoolMemory();
        static MemoryPoolCache cache(mem_pool, mem_pool->GetSize() / sizeof(UnclaimedTransaction));
        return &cache;
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

    bool UnclaimedTransactionPool::HasUnclaimedTransaction(const uint256_t& hash){
        LOCK_GUARD;
        return GetCache()->ContainsItem(hash)
            || GetJournal()->HasData(hash);
    }

    Handle<UnclaimedTransaction> UnclaimedTransactionPool::GetUnclaimedTransaction(const uint256_t& hash){
        LOCK_GUARD;
        if(GetCache()->ContainsItem(hash)){
            return (UnclaimedTransaction*)GetCache()->GetItem(hash);
        }

        if(GetJournal()->HasData(hash)){
            Handle<UnclaimedTransaction> utxo = GetJournal()->GetData(hash);
            if(!GetCache()->PutItem(utxo)) LOG(WARNING) << "cannot put unclaimed transaction " << hash << " into pool cache";
            return utxo;
        }

        return nullptr;
    }

    bool UnclaimedTransactionPool::RemoveUnclaimedTransaction(const uint256_t& hash){
        LOCK_GUARD;
        if(GetCache()->ContainsItem(hash))
            GetCache()->RemoveItem(hash);
        if(!GetJournal()->RemoveData(hash))
            return false;
        LOG(INFO) << "removed unclaimed transaction " << hash << " from pool";
        return true;
    }

    bool UnclaimedTransactionPool::PutUnclaimedTransaction(const Handle<UnclaimedTransaction>& utxo){
        LOCK_GUARD;
        uint256_t hash = utxo->GetHash();
        if(GetCache()->ContainsItem(hash) || GetJournal()->HasData(hash))
            return false;
        if(!GetCache()->PutItem(utxo)) {
            LOG(WARNING) << "couldn't put unclaimed transaction " << hash << " in pool cache";
            return false;
        }

        if(!GetJournal()->PutData(utxo)){
            LOG(WARNING) << "couldn't write unclaimed transaction " << hash << " to disk";
            if(!GetCache()->RemoveItem(hash)){
                LOG(WARNING) << "couldn't remove unclaimed transaction " << hash << " from pool cache";
                return false;
            }
            return false;
        }

        return true;
    }

    size_t UnclaimedTransactionPool::GetSize(){
        LOCK_GUARD;
        return GetJournal()->GetNumberOfObjects();
    }

    size_t UnclaimedTransactionPool::GetCacheSize(){
        LOCK_GUARD;
        return GetCache()->GetSize();
    }

    size_t UnclaimedTransactionPool::GetMaxCacheSize(){
        LOCK_GUARD;
        return GetCache()->GetMaxSize();
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

    Handle<UnclaimedTransaction> UnclaimedTransactionPool::GetUnclaimedTransaction(const uint256_t &tx_hash, uint32_t tx_index){
        LOG(INFO) << "searching for: " << tx_hash << "[" << tx_index << "]....";

        LOCK_GUARD;
        DIR* dir;
        struct dirent* ent;
        if((dir = opendir(GetPath().c_str())) != NULL){
            while((ent = readdir(dir)) != NULL){
                std::string name(ent->d_name);
                std::string filename = (GetPath() + "/" + name);
                if(!EndsWith(filename, ".dat")) continue;

                Handle<UnclaimedTransaction> utxo = UnclaimedTransaction::NewInstance(filename);
                if(utxo->GetTransaction() == tx_hash) LOG(INFO) << "checking: " << utxo;
                if(utxo->GetTransaction() == tx_hash && utxo->GetIndex() == tx_index)
                    return utxo;
            }
            closedir(dir);
        }
        LOG(WARNING) << "couldn't find unclaimed transaction: " << tx_hash << "[" << tx_index << "]";
        return nullptr;
    }

    class UnclaimedTransactionPoolPrinter : public ObjectPointerVisitor, public UnclaimedTransactionPoolVisitor{
    public:
        UnclaimedTransactionPoolPrinter(): UnclaimedTransactionPoolVisitor(){}
        ~UnclaimedTransactionPoolPrinter() = default;

        bool Visit(RawObject* obj){
            Handle<UnclaimedTransaction> utxo = ((UnclaimedTransaction*)obj);
            return Visit(utxo);
        }

        bool Visit(const Handle<UnclaimedTransaction>& utxo){
            LOG(INFO) << utxo->GetTransaction() << "[" << utxo->GetIndex() << "] := " << utxo->GetHash();
            return true;
        }
    };

    bool UnclaimedTransactionPool::Print(bool cache_only){
        UnclaimedTransactionPoolPrinter printer;
        return cache_only
             ? GetCache()->Accept(&printer)
             : Accept(&printer);
    }
}