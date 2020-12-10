#include <mutex>
#include <condition_variable>
#include <leveldb/db.h>
#include "buffer.h"
#include "unclaimed_transaction.h"

namespace Token{
    UnclaimedTransactionPtr UnclaimedTransaction::NewInstance(std::fstream& fd, size_t size){
        Buffer buff(size);
        fd.read((char*)buff.data(), size);
        return UnclaimedTransactionPtr(new UnclaimedTransaction(&buff));
    }

    std::string UnclaimedTransaction::ToString() const{
        std::stringstream stream;
        stream << "UnclaimedTransaction(" << hash_ << "[" << index_ << "])";
        return stream.str();
    }

    static inline std::string
    GetDataDirectory(){
        return FLAGS_path + "/utxos";
    }

    static inline std::string
    GetIndexFilename(){
        return GetDataDirectory() + "/index";
    }

    static inline std::string
    GetNewDataFilename(const Hash& hash){
        std::string hashString = hash.HexString();
        std::string front = hashString.substr(0, 8);
        std::string tail = hashString.substr(hashString.length() - 8, hashString.length());
        std::string filename = GetDataDirectory() + "/" + front + ".dat";
        if(FileExists(filename)){
            filename = GetDataDirectory() + "/" + tail + ".dat";
        }
        return filename;
    }

    static std::mutex mutex_;
    static std::condition_variable cond_;
    static UnclaimedTransactionPool::State state_ = UnclaimedTransactionPool::kUninitialized;
    static UnclaimedTransactionPool::Status status_ = UnclaimedTransactionPool::kOk;
    static leveldb::DB* index_ = nullptr;

#define POOL_OK_STATUS(Status, Message) \
    LOG(INFO) << (Message);             \
    SetStatus((Status));
#define POOL_OK(Message) \
    POOL_OK_STATUS(UnclaimedTransactionPool::kOk, (Message));
#define POOL_WARNING_STATUS(Status, Message) \
    LOG(WARNING) << (Message);               \
    SetStatus((Status));
#define POOL_WARNING(Message) \
    POOL_WARNING_STATUS(UnclaimedTransactionPool::kWarning, (Message))
#define POOL_ERROR_STATUS(Status, Message) \
    LOG(ERROR) << (Message);               \
    SetStatus((Status));
#define POOL_ERROR(Message) \
    POOL_ERROR_STATUS(UnclaimedTransactionPool::kError, (Message))

#define LOCK_GUARD std::lock_guard<std::mutex> guard(mutex_)
#define LOCK std::unique_lock<std::mutex> lock(mutex_)
#define WAIT cond_.wait(lock)
#define SIGNAL_ONE cond_.notify_one()
#define SIGNAL_ALL cond_.notify_all()

    static inline leveldb::DB*
    GetIndex(){
        return index_;
    }

    static inline std::string
    GetDataFilename(const Hash& hash){
        leveldb::ReadOptions options;
        std::string key = hash.HexString();
        std::string filename;
        return GetIndex()->Get(options, key, &filename).ok() ?
                   filename :
                   GetNewDataFilename(hash);
    }

    std::string UnclaimedTransactionPool::GetStatusMessage(){
        std::stringstream ss;
        LOCK_GUARD;

        ss << "[";
        switch(state_){
#define DEFINE_STATE_MESSAGE(Name) \
            case UnclaimedTransactionPool::k##Name: \
                ss << #Name; \
                break;
            FOR_EACH_UTXOPOOL_STATE(DEFINE_STATE_MESSAGE)
#undef DEFINE_STATE_MESSAGE
        }
        ss << "] ";

        switch(status_){
#define DEFINE_STATUS_MESSAGE(Name) \
            case UnclaimedTransactionPool::k##Name:{ \
                ss << #Name; \
                break; \
            }
            FOR_EACH_UTXOPOOL_STATUS(DEFINE_STATUS_MESSAGE)
#undef DEFINE_STATUS_MESSAGE
        }
        return ss.str();
    }

    void UnclaimedTransactionPool::SetState(State state){
        LOCK_GUARD;
        state_ = state;
        SIGNAL_ALL;
    }

    UnclaimedTransactionPool::State UnclaimedTransactionPool::GetState(){
        LOCK_GUARD;
        return state_;
    }

    void UnclaimedTransactionPool::SetStatus(Status status){
        LOCK_GUARD;
        status_ = status;
        SIGNAL_ALL;
    }

    UnclaimedTransactionPool::Status UnclaimedTransactionPool::GetStatus(){
        LOCK_GUARD;
        return status_;
    }

    bool UnclaimedTransactionPool::Initialize(){
        if(IsInitialized()){
            POOL_WARNING("cannot re-initialize the unclaimed transaction pool.");
            return false;
        }

        LOG(INFO) << "initializing the unclaimed transaction pool in: " << GetDataDirectory();
        SetState(UnclaimedTransactionPool::kInitializing);
        if(!FileExists(GetDataDirectory())){
            if(!CreateDirectory(GetDataDirectory())){
                std::stringstream ss;
                ss << "couldn't create the unclaimed transaction pool directory: " << GetDataDirectory();
                POOL_ERROR(ss.str());
                return false;
            }
        }

        leveldb::Options options;
        options.create_if_missing = true;
        leveldb::Status status = leveldb::DB::Open(options, GetIndexFilename(), &index_);
        if(!status.ok()){
            std::stringstream ss;
            ss << "couldn't initialize the unclaimed transaction pool index: " << status.ToString();
            POOL_ERROR(ss.str());
            return false;
        }

        SetState(UnclaimedTransactionPool::kInitialized);
        POOL_OK("unclaimed transaction pool initialized.");
        return true;
    }

    UnclaimedTransactionPtr UnclaimedTransactionPool::GetUnclaimedTransaction(const Hash& hash){
        leveldb::ReadOptions options;
        std::string key = hash.HexString();
        std::string filename;

        LOCK_GUARD;
        leveldb::Status status = GetIndex()->Get(options, key, &filename);
        if(!status.ok()){
            std::stringstream ss;
            ss << "couldn't find block " << hash << " in index: " << status.ToString();
            POOL_WARNING(ss.str());
            return nullptr;
        }

        UnclaimedTransactionPtr utxo = UnclaimedTransaction::NewInstance(filename);
        if(hash != utxo->GetHash()){
            std::stringstream ss;
            ss << "couldn't verify unclaimed transaction hash: " << hash;
            POOL_WARNING(ss.str());
            //TODO: better validation + error handling for UnclaimedTransaction data
            return nullptr;
        }
        return utxo;
    }

    bool UnclaimedTransactionPool::PutUnclaimedTransaction(const Hash& hash, const UnclaimedTransactionPtr& utxo){
        leveldb::WriteOptions options;
        options.sync = true;
        std::string key = hash.HexString();
        std::string filename = GetNewDataFilename(hash);

        LOCK_GUARD;
        if(FileExists(filename)){
            std::stringstream ss;
            ss << "cannot overwrite existing unclaimed transaction pool data: " << filename;
            POOL_ERROR(ss.str());
            return false;
        }

        if(!utxo->WriteToFile(filename)){
            std::stringstream ss;
            ss << "cannot write unclaimed transaction pool data to: " << filename;
            POOL_ERROR(ss.str());
            return false;
        }

        leveldb::Status status = GetIndex()->Put(options, key, filename);
        if(!status.ok()){
            std::stringstream ss;
            ss << "couldn't index unclaimed transaction " << utxo << ": " << status.ToString();
            POOL_ERROR(ss.str());
            return false;
        }

        LOG(INFO) << "added unclaimed transaction to pool: " << hash;
        return true;
    }

    bool UnclaimedTransactionPool::HasUnclaimedTransaction(const Hash& hash){
        leveldb::ReadOptions options;
        std::string key = hash.HexString();
        std::string filename;

        LOCK_GUARD;
        leveldb::Status status = GetIndex()->Get(options, key, &filename);
        if(!status.ok()){
            std::stringstream ss;
            ss << "couldn't find unclaimed transaction " << hash << ": " << status.ToString();
            POOL_WARNING(ss.str());
            return false;
        }
        return true;
    }

    bool UnclaimedTransactionPool::RemoveUnclaimedTransaction(const Hash& hash){
        if(!HasUnclaimedTransaction(hash)){
            std::stringstream ss;
            ss << "cannot remove non-existing unclaimed transaction: " << hash;
            POOL_WARNING(ss.str());
            return false;
        }

        leveldb::WriteOptions options;
        options.sync = true;
        std::string key = hash.HexString();
        std::string filename = GetDataFilename(hash);

        leveldb::Status status = GetIndex()->Delete(options, key);
        if(!status.ok()){
            std::stringstream ss;
            ss << "couldn't remove unclaimed transaction " << hash << " from pool: " << status.ToString();
            POOL_ERROR(ss.str());
            return false;
        }

        if(!DeleteFile(filename)){
            std::stringstream ss;
            ss << "couldn't remove the unclaimed transaction file: " << filename;
            POOL_ERROR(ss.str());
            return false;
        }

        LOG(INFO) << "removed unclaimed transaction: " << hash;
        return true;
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
                UnclaimedTransactionPtr utxo = UnclaimedTransaction::NewInstance(filename);
                if(!vis->Visit(utxo))
                    break;
            }
            closedir(dir);
        }

        return vis->VisitEnd();
    }

    bool UnclaimedTransactionPool::GetUnclaimedTransactions(std::vector<Hash>& utxos){
        //TODO: better error handling and validation
        LOCK_GUARD;
        DIR* dir;
        struct dirent* ent;
        if((dir = opendir(GetDataDirectory().c_str())) != NULL){
            while((ent = readdir(dir)) != NULL){
                std::string name(ent->d_name);
                std::string filename = (GetDataDirectory() + "/" + name);
                if(!EndsWith(filename, ".dat")) continue;

                UnclaimedTransactionPtr utxo = UnclaimedTransaction::NewInstance(filename);
                utxos.push_back(utxo->GetHash());
            }
            closedir(dir);
            return true;
        }

        return false;
    }

    bool UnclaimedTransactionPool::GetUnclaimedTransactions(const std::string& user, std::vector<Hash>& utxos){
        //TODO: better error handling + validation
        LOCK_GUARD;
        DIR* dir;
        struct dirent* ent;
        if((dir = opendir(GetDataDirectory().c_str())) != NULL){
            while((ent = readdir(dir)) != NULL){
                std::string name(ent->d_name);
                std::string filename = (GetDataDirectory() + "/" + name);
                if(!EndsWith(filename, ".dat")) continue;
                UnclaimedTransactionPtr utxo = UnclaimedTransaction::NewInstance(filename);

                if(utxo->GetUser() != user) continue;
                utxos.push_back(utxo->GetHash());
            }
            closedir(dir);
            return true;
        }
        return false;
    }

    UnclaimedTransactionPtr UnclaimedTransactionPool::GetUnclaimedTransaction(const Hash &tx_hash, int32_t tx_index){
        //TODO: better error handling + validation
        LOCK_GUARD;
        DIR* dir;
        struct dirent* ent;
        if((dir = opendir(GetDataDirectory().c_str())) != NULL){
            while((ent = readdir(dir)) != NULL){
                std::string name(ent->d_name);
                std::string filename = (GetDataDirectory() + "/" + name);
                if(!EndsWith(filename, ".dat")) continue;

                UnclaimedTransactionPtr utxo = UnclaimedTransaction::NewInstance(filename);
                if(utxo->GetTransaction() == tx_hash && utxo->GetIndex() == tx_index)
                    return utxo;
            }
            closedir(dir);
        }
        LOG(WARNING) << "couldn't find unclaimed transaction: " << tx_hash << "[" << tx_index << "]";
        return nullptr;
    }

    int64_t UnclaimedTransactionPool::GetNumberOfUnclaimedTransactions(){
        //TODO: better error handling and validation
        int64_t count = 0;
        DIR* dir;
        struct dirent* ent;
        if((dir = opendir(GetDataDirectory().c_str())) != NULL){
            while((ent = readdir(dir)) != NULL){
                std::string filename(ent->d_name);
                if(EndsWith(filename, ".dat")) count++;
            }
            closedir(dir);
        } else{
            LOG(WARNING) << "couldn't list files in object cache directory: " << GetDataDirectory();
            return false;
        }
        return count;
    }

    class UnclaimedTransactionPoolPrinter : public UnclaimedTransactionPoolVisitor{
    public:
        UnclaimedTransactionPoolPrinter(): UnclaimedTransactionPoolVisitor(){}
        ~UnclaimedTransactionPoolPrinter() = default;

        bool Visit(const UnclaimedTransactionPtr& utxo){
            LOG(INFO) << utxo->GetTransaction() << "[" << utxo->GetIndex() << "] := " << utxo->GetHash();
            return true;
        }
    };

    bool UnclaimedTransactionPool::Print(){
        UnclaimedTransactionPoolPrinter printer;
        return Accept(&printer);
    }
}