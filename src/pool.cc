#include <mutex>
#include <glog/logging.h>
#include <condition_variable>
#include "pool.h"
#include "object_tag.h"

namespace Token{
    static std::mutex mutex_;
    static std::condition_variable cond_;
    static ObjectPool::State state_ = ObjectPool::kUninitialized;
    static ObjectPool::Status status_ = ObjectPool::kOk;
    static leveldb::DB* index_ = nullptr;

#define LOCK_GUARD std::lock_guard<std::mutex> guard(mutex_)
#define LOCK std::unique_lock<std::mutex> lock(mutex_)
#define UNLOCK lock.unlock()
#define WAIT cond_.wait(lock)
#define SIGNAL_ONE cond_.notify_one()
#define SIGNAL_ALL cond_.notify_all()

    static inline leveldb::DB*
    GetIndex(){
        return index_;
    }

    static inline std::string
    GetDataDirectory(){
        return TOKEN_BLOCKCHAIN_HOME + "/pool";
    }

    static inline std::string
    GetIndexFilename(){
        return GetDataDirectory() + "/index";
    }

    static inline std::string
    GetDataFilename(const Hash& hash){
        std::stringstream filename;
        filename << GetDataDirectory() << "/" << hash << ".dat";
        return filename.str();
    }

    ObjectPool::State ObjectPool::GetState(){
        LOCK_GUARD;
        return state_;
    }

    void ObjectPool::SetState(const State& state){
        LOCK;
        state_ = state;
        UNLOCK;
        SIGNAL_ALL;
    }

    ObjectPool::Status ObjectPool::GetStatus(){
        LOCK_GUARD;
        return status_;
    }

    void ObjectPool::SetStatus(const Status& status){
        LOCK;
        status_ = status;
        UNLOCK;
        SIGNAL_ALL;
    }

    bool ObjectPool::Initialize(){
        if(IsInitialized()){
            LOG(WARNING) << "cannot re-initialize the object pool.";
            return false;
        }

        LOG(INFO) << "initializing the object pool....";
        SetState(ObjectPool::kInitializing);
        if(!FileExists(GetDataDirectory())){
            if(!CreateDirectory(GetDataDirectory())){
                LOG(WARNING) << "cannot create the object pool directory: " << GetDataDirectory();
                return false;
            }
        }

        leveldb::Options options;
        options.create_if_missing = true;

        leveldb::Status status;
        if(!(status = leveldb::DB::Open(options, GetIndexFilename(), &index_)).ok()){
            LOG(WARNING) << "couldn't initialize the object pool index: " << status.ToString();
            return false;
        }

        LOG(INFO) << "object pool initialized.";
        SetState(ObjectPool::kInitialized);
        return true;
    }

    bool ObjectPool::HasObject(const Hash& hash){
        leveldb::ReadOptions readOpts;
        std::string key = hash.HexString();
        std::string filename;
        return GetIndex()->Get(readOpts, key, &filename).ok();
    }

    bool ObjectPool::HasObjectByType(const Hash& hash, const ObjectTag::Type& type){
        leveldb::ReadOptions readOpts;
        std::string key = hash.HexString();
        std::string filename;

        leveldb::Status status;
        if((status = GetIndex()->Get(readOpts, key, &filename)).IsNotFound())
            return false;
        ObjectTagVerifier verifier(filename, type);
        return verifier.IsValid();
    }

    bool ObjectPool::WaitForObject(const Hash& hash){
        LOCK;
        while(!HasObject(hash)) WAIT;
        UNLOCK;
        return true;
    }

    bool ObjectPool::RemoveObject(const Hash& hash){
        if(!HasObject(hash)){
            LOG(WARNING) << "cannot remove non-existent object " << hash << " from pool.";
            return false;
        }

        leveldb::WriteOptions writeOpts;
        writeOpts.sync = true;

        std::string key = hash.HexString();
        std::string filename = GetDataFilename(hash);

        leveldb::Status status;
        if(!(status = GetIndex()->Delete(writeOpts, key)).ok()){
            LOG(WARNING) << "couldn't remove object " << hash << " from pool: " << status.ToString();
            SetStatus(ObjectPool::kWarning);
            return false;
        }

        if(!DeleteFile(filename)){
            LOG(WARNING) << "couldn't remove object pool file: " << filename;
            SetStatus(ObjectPool::kWarning);
            return false;
        }

        LOG(INFO) << "remove object " << hash << " from pool.";
        return true;
    }

    leveldb::Status ObjectPool::IndexObject(const Hash& hash, const std::string& filename){
        leveldb::WriteOptions writeOpts;
        writeOpts.sync = true;
        return GetIndex()->Put(writeOpts, hash.HexString(), filename);
    }

    bool ObjectPool::PutObject(const Hash& hash, const BlockPtr& val){
        std::string filename = GetDataFilename(hash);
        if(HasObject(hash) || FileExists(filename)){
            LOG(WARNING) << "cannot overwrite existing object pool data for: " << hash << "(" << filename << ")";
            SetStatus(ObjectPool::kWarning);
            return false;
        }

        leveldb::Status status;
        if(!(status = IndexObject(hash, filename)).ok()){
            LOG(WARNING) << "cannot index object pool data for " << hash << ": " << status.ToString();
            SetStatus(ObjectPool::kWarning);
            return false;
        }

        if(!WriteObject<Block, BlockFileWriter>(val, filename)){
            LOG(WARNING) << "couldn't write object data to: " << filename;
            SetStatus(ObjectPool::kWarning);
            return false;
        }

        //TODO: should we signal
        LOG(INFO) << "indexed object: " << hash << "(" << filename << ")";
        return true;
    }

    bool ObjectPool::PutObject(const Hash& hash, const TransactionPtr& val){
        std::string filename = GetDataFilename(hash);
        if(HasObject(hash) || FileExists(filename)){
            LOG(WARNING) << "cannot overwrite existing object pool data for: " << hash << "(" << filename << ")";
            SetStatus(ObjectPool::kWarning);
            return false;
        }

        leveldb::Status status;
        if(!(status = IndexObject(hash, filename)).ok()){
            LOG(WARNING) << "cannot index object pool data for " << hash << ": " << status.ToString();
            SetStatus(ObjectPool::kWarning);
            return false;
        }

        if(!WriteObject<Transaction, TransactionFileWriter>(val, filename)){
            LOG(WARNING) << "couldn't write object data to: " << filename;
            SetStatus(ObjectPool::kWarning);
            return false;
        }

        //TODO: should we signal
        LOG(INFO) << "indexed object: " << hash << "(" << filename << ")";
        return true;
    }

    bool ObjectPool::PutObject(const Hash& hash, const UnclaimedTransactionPtr& val){
        std::string filename = GetDataFilename(hash);
        if(HasObject(hash) || FileExists(filename)){
            LOG(WARNING) << "cannot overwrite existing object pool data for: " << hash << "(" << filename << ")";
            SetStatus(ObjectPool::kWarning);
            return false;
        }

        leveldb::Status status;
        if(!(status = IndexObject(hash, filename)).ok()){
            LOG(WARNING) << "cannot index object pool data for " << hash << ": " << status.ToString();
            SetStatus(ObjectPool::kWarning);
            return false;
        }

        if(!WriteObject<UnclaimedTransaction, UnclaimedTransactionFileWriter>(val, filename)){
            LOG(WARNING) << "couldn't write object data to: " << filename;
            SetStatus(ObjectPool::kWarning);
            return false;
        }

        //TODO: should we signal
        LOG(INFO) << "indexed object: " << hash << "(" << filename << ")";
        return true;
    }

    bool ObjectPool::GetBlocks(HashList& hashes){
        return false;
    }

    bool ObjectPool::GetTransactions(HashList& hashes){
        return false;
    }

    bool ObjectPool::GetUnclaimedTransactions(HashList& hashes){
        return false;
    }

    bool ObjectPool::VisitBlocks(ObjectPoolBlockVisitor* vis){
        if(!vis)
            return false;

        return false;
    }

    bool ObjectPool::VisitTransactions(ObjectPoolTransactionVisitor* vis){
        if(!vis)
            return false;
        return false;
    }

    bool ObjectPool::VisitUnclaimedTransactions(ObjectPoolUnclaimedTransactionVisitor* vis){
        if(!vis)
            return false;
        return false;
    }

    BlockPtr ObjectPool::GetBlock(const Hash& hash){
        return nullptr;
    }

    TransactionPtr ObjectPool::GetTransaction(const Hash& hash){
        return nullptr;
    }

    UnclaimedTransactionPtr ObjectPool::GetUnclaimedTransaction(const Hash& hash){
        return nullptr;
    }

    int64_t ObjectPool::GetNumberOfObjects(){
        int64_t count = 0;

        DIR* dir;
        struct dirent* ent;
        if((dir = opendir(GetDataDirectory().c_str())) != NULL){
            while((ent = readdir(dir)) != NULL){
                std::string name(ent->d_name);
                std::string filename = (GetDataDirectory() + "/" + name);
                if(!EndsWith(filename, ".dat"))
                    continue;

                // matches everything
                ObjectTagVerifier tag_verifier(filename, ObjectTag::kNone);
                if(!tag_verifier.IsValid())
                    continue;
                count++;
            }
            closedir(dir);
        }
        return count;
    }

    int64_t ObjectPool::GetNumberOfObjectsByType(const ObjectTag::Type& type){
        int64_t count = 0;

        DIR* dir;
        struct dirent* ent;
        if((dir = opendir(GetDataDirectory().c_str())) != NULL){
            while((ent = readdir(dir)) != NULL){
                std::string name(ent->d_name);
                std::string filename = (GetDataDirectory() + "/" + name);
                if(!EndsWith(filename, ".dat"))
                    continue;

                ObjectTagVerifier tag_verifier(filename, type);
                if(!tag_verifier.IsValid())
                    continue;
                count++;
            }
            closedir(dir);
        }
        return count;
    }
}