#include <glog/logging.h>
#include <unordered_map>
#include "crash_report.h"
#include "transaction_pool.h"
#include "transaction_pool_cache.h"
#include "transaction_pool_index.h"

namespace Token{
    static std::recursive_mutex mutex_;
    static TransactionPool::State state_;

#define LOCK_GUARD std::lock_guard<std::recursive_mutex> guard(mutex_);

    TransactionPool::State TransactionPool::GetState(){
        LOCK_GUARD;
        return state_;
    }

    void TransactionPool::SetState(TransactionPool::State state){
        LOCK_GUARD;
        state_ = state;
    }

    void TransactionPool::Initialize(){
        if(!IsUninitialized()){
            std::stringstream ss;
            ss << "Cannot reinitialize the transaction pool";
            CrashReport::GenerateAndExit(ss);
        }

        SetState(State::kInitializing);
        TransactionPoolIndex::Initialize();
        SetState(State::kInitialized);
#ifdef TOKEN_DEBUG
        LOG(INFO) << "initialized the transaction pool";
#endif//TOKEN_DEBUG
    }

    bool TransactionPool::HasTransaction(const uint256_t& hash){
        LOCK_GUARD;
        return TransactionPoolIndex::HasData(hash);
    }

    Handle<Transaction> TransactionPool::GetTransaction(const uint256_t& hash){
        LOCK_GUARD;
        if(!TransactionPoolCache::HasTransaction(hash)){
            if(!TransactionPoolIndex::HasData(hash)) return Handle<Transaction>(); //null

#ifdef TOKEN_DEBUG
            LOG(INFO) << "loading transaction: " << hash;
#endif//TOKEN_DEBUG

            Handle<Transaction> tx = TransactionPoolIndex::GetData(hash);
            if(!TransactionPoolCache::HasTransaction(hash)){
                if(TransactionPoolCache::IsFull()) {
                    TransactionPoolCache::EvictLastUsed();
                }
            } else{
                TransactionPoolCache::Promote(hash);
            }
            return tx;
        }
        return TransactionPoolCache::GetTransaction(hash);
    }

    void TransactionPool::RemoveTransaction(const uint256_t& hash){
        LOCK_GUARD;
        //TODO: implement
        if(!HasTransaction(hash)) return;
    }

    void TransactionPool::PutTransaction(const Handle<Transaction>& tx){
        LOCK_GUARD;
        uint256_t hash = tx->GetSHA256Hash();
        if(!TransactionPoolCache::HasTransaction(hash)){
            if(TransactionPoolCache::IsFull()) {
                TransactionPoolCache::EvictLastUsed();
            }
        } else{
            TransactionPoolCache::Promote(hash);
        }
        TransactionPoolCache::PutTransaction(hash, tx);
    }

    bool TransactionPool::GetTransactions(std::vector<uint256_t>& txs){
        LOCK_GUARD;

        DIR* dir;
        struct dirent* ent;
        if((dir = opendir(TransactionPoolIndex::GetDirectory().c_str())) != NULL){
            while((ent = readdir(dir)) != NULL){
                std::string name(ent->d_name);
                std::string filename = (TransactionPoolIndex::GetDirectory() + "/" + name);
                if(!EndsWith(filename, ".dat")) continue;

                Transaction* tx = Transaction::NewInstance(filename);
                txs.push_back(tx->GetSHA256Hash());
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
        if((dir = opendir(TransactionPoolIndex::GetDirectory().c_str())) != NULL){
            while((ent = readdir(dir)) != NULL){
                std::string name(ent->d_name);
                std::string filename = (TransactionPoolIndex::GetDirectory() + "/" + name);
                if(!EndsWith(filename, ".dat")) continue;
                size++;
            }
            closedir(dir);
        }
        return size;
    }
}