#include <leveldb/db.h>
#include <glog/logging.h>
#include "transaction_pool.h"
#include "crash_report.h"

namespace Token{
    //######################################################################################################################
//                                          Transaction Pool
//######################################################################################################################
    static std::recursive_mutex mutex_;
    static TransactionPool::State state_;
    static leveldb::DB* index_ = nullptr;

#define LOCK_GUARD std::lock_guard<std::recursive_mutex> guard(mutex_);

    static inline std::string
    GetDataDirectory(){
        return FLAGS_path + "/txs";
    }

    static inline std::string
    GetIndexFilename(){
        return GetDataDirectory() + "/index";
    }

    static inline leveldb::DB*
    GetIndex(){
        return index_;
    }

    static inline std::string
    GetNewTransactionFilename(const uint256_t& hash){
        std::string hashString = HexString(hash);
        std::string front = hashString.substr(0, 8);
        std::string tail = hashString.substr(hashString.length() - 8, hashString.length());
        std::string filename = GetDataDirectory() + "/" + front + ".dat";
        if(FileExists(filename)){
            filename = GetDataDirectory() + "/" + tail + ".dat";
        }
        return filename;
    }

    static inline std::string
    GetTransactionFilename(const uint256_t& hash){
        leveldb::ReadOptions options;
        std::string key = HexString(hash);
        std::string filename;
        if(!GetIndex()->Get(options, key, &filename).ok()) return GetNewTransactionFilename(hash);
        return filename;
    }

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
        std::string filename = GetDataDirectory();
        if(!FileExists(filename)){
            if(!CreateDirectory(filename)){
                std::stringstream ss;
                ss << "Cannot create the TransactionPool data directory: " << filename;
                CrashReport::GenerateAndExit(ss);
            }
        }

        leveldb::Options options;
        options.create_if_missing = true;
        if(!leveldb::DB::Open(options, GetIndexFilename(), &index_).ok()){
            std::stringstream ss;
            ss << "Cannot create TransactionPool index: " << GetIndexFilename();
        }

        SetState(State::kInitialized);
#ifdef TOKEN_DEBUG
        LOG(INFO) << "initialized the transaction pool";
#endif//TOKEN_DEBUG
    }

    bool TransactionPool::HasTransaction(const uint256_t& hash){
        leveldb::ReadOptions options;
        std::string key = HexString(hash);
        std::string filename;
        LOCK_GUARD;
        return GetIndex()->Get(options, key, &filename).ok();
    }

    Handle<Transaction> TransactionPool::GetTransaction(const uint256_t& hash){
        if(!HasTransaction(hash)) return nullptr;
        LOCK_GUARD;
        std::string filename = GetTransactionFilename(hash);
        return Transaction::NewInstance(filename);
    }

    bool TransactionPool::GetTransactions(std::vector<uint256_t>& txs){
        LOCK_GUARD;
        DIR* dir;
        struct dirent* ent;
        if((dir = opendir(GetDataDirectory().c_str())) != NULL){
            while((ent = readdir(dir)) != NULL){
                std::string name(ent->d_name);
                std::string filename = (GetDataDirectory() + "/" + name);
                if(!EndsWith(filename, ".dat")) continue;
                Transaction* tx = Transaction::NewInstance(filename);
                txs.push_back(tx->GetSHA256Hash());
            }
            closedir(dir);
            return true;
        }
        return false;
    }

    void TransactionPool::RemoveTransaction(const uint256_t& hash){
        if(!HasTransaction(hash)) return;
        LOCK_GUARD;
        std::string filename = GetTransactionFilename(hash);
        if(!DeleteFile(filename)){
            std::stringstream ss;
            ss << "Couldn't remove transaction " << hash << " data file: " << filename;
            CrashReport::GenerateAndExit(ss);
        }

        leveldb::WriteOptions options;
        if(!GetIndex()->Delete(options, HexString(hash)).ok()){
            std::stringstream ss;
            ss << "Couldn't remove transaction " << hash << " from index";
            CrashReport::GenerateAndExit(ss);
        }
    }

    void TransactionPool::PutTransaction(Transaction* tx){
        uint256_t hash = tx->GetSHA256Hash();
        if(HasTransaction(hash)){
            std::stringstream ss;
            ss << "Couldn't add duplicate transaction " << hash << " to transaction pool";
            CrashReport::GenerateAndExit(ss);
        }

        LOCK_GUARD;
        std::string filename = GetNewTransactionFilename(hash);
        std::fstream fd(filename, std::ios::out|std::ios::binary);
        if(!tx->WriteToFile(fd)){
            std::stringstream ss;
            ss << "Couldn't write transaction " << hash << " to file: " << filename;
            CrashReport::GenerateAndExit(ss);
        }

        leveldb::WriteOptions options;
        std::string key = HexString(hash);
        if(!GetIndex()->Put(options, key, filename).ok()){
            std::stringstream ss;
            ss << "Couldn't index transaction: " << hash;
            CrashReport::GenerateAndExit(ss);
        }
    }

    uint32_t TransactionPool::GetNumberOfTransactions(){
        std::vector<uint256_t> txs;
        if(!GetTransactions(txs)) return 0;
        return txs.size();
    }
}