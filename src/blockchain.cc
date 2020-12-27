#include <mutex>
#include <sstream>
#include <leveldb/db.h>
#include <glog/logging.h>
#include <condition_variable>
#include "pool.h"
#include "keychain.h"
#include "blockchain.h"
#include "job/scheduler.h"
#include "job/process_block.h"
#include "unclaimed_transaction.h"

namespace Token{
    static inline bool
    ShouldLoadSnapshot(){
        return !FLAGS_snapshot.empty();
    }

    static inline std::string
    GetDataDirectory(){
        return FLAGS_path + "/data";
    }

    static inline std::string
    GetIndexFilename(){
        return GetDataDirectory() + "/index";
    }

    static inline std::string
    GetNewBlockFilename(const BlockPtr& blk){
        std::stringstream ss;
        ss << GetDataDirectory() + "/blk" << blk->GetHeight() << ".dat";
        return ss.str();
    }

    static inline std::string
    GetBlockHeightKey(int64_t height){
        std::stringstream ss;
        ss << "blk" << height;
        return ss.str();
    }

    static std::recursive_mutex mutex_;
    static std::condition_variable cond_;
    static BlockChain::State state_ = BlockChain::kUninitialized;
    static BlockChain::Status status_ = BlockChain::kOk;
    static leveldb::DB* index_ = nullptr;

#define LOCK_GUARD std::lock_guard<std::recursive_mutex> guard(mutex_)
#define LOCK std::unique_lock<std::recursive_mutex> lock(mutex_)
#define WAIT cond_.wait(lock)
#define SIGNAL_ONE cond_.notify_one()
#define SIGNAL_ALL cond_.notify_all()

    std::string BlockChain::GetStatusMessage(){
        std::stringstream ss;
        LOCK_GUARD;

        ss << "[";
        switch(state_){
#define DEFINE_STATE_MESSAGE(Name) \
            case BlockChain::k##Name: \
                ss << #Name; \
                break;
            FOR_EACH_BLOCKCHAIN_STATE(DEFINE_STATE_MESSAGE)
#undef DEFINE_STATE_MESSAGE
        }
        ss << "] ";

        switch(status_){
#define DEFINE_STATUS_MESSAGE(Name) \
            case BlockChain::k##Name:{ \
                ss << #Name; \
                break; \
            }
            FOR_EACH_BLOCKCHAIN_STATUS(DEFINE_STATUS_MESSAGE)
#undef DEFINE_STATUS_MESSAGE
        }
        return ss.str();
    }

    leveldb::DB* BlockChain::GetIndex(){
        return index_;
    }

    bool BlockChain::Initialize(){
        if(IsInitialized()){
            LOG(WARNING) << "cannot reinitialize the block chain!";
            return false;
        }

        if(!FileExists(GetDataDirectory())){
            if(!CreateDirectory(GetDataDirectory())){
                LOG(WARNING) << "couldn't create block chain index in directory: " << GetDataDirectory();
                return false;
            }
        }

        leveldb::Options options;
        options.create_if_missing = true;
        if(!leveldb::DB::Open(options, GetIndexFilename(), &index_).ok()){
            LOG(WARNING) << "couldn't initialize block chain index in file: " << GetIndexFilename();
            return false;
        }

        /*if(ShouldLoadSnapshot()){
            Snapshot* snapshot = nullptr;
            if(!(snapshot = Snapshot::ReadSnapshot(FLAGS_snapshot))){
                LOG(WARNING) << "couldn't load snapshot: " << FLAGS_snapshot;
                return false;
            }

            SnapshotBlockChainInitializer initializer(snapshot);
            if(!initializer.Initialize()){
                LOG(WARNING) << "couldn't initialize block chain from snapshot: " << FLAGS_snapshot;
                return false;
            }
            return true;
        }*/

        LOG(INFO) << "initializing the block chain....";
        SetState(BlockChain::kInitializing);

        Keychain::Initialize();
        ObjectPool::Initialize();
        if(!HasBlocks()){
            BlockPtr genesis = Block::Genesis();
            Hash hash = genesis->GetHash();
            PutBlock(hash, genesis);
            PutReference(BLOCKCHAIN_REFERENCE_HEAD, hash);
            PutReference(BLOCKCHAIN_REFERENCE_GENESIS, hash);

            //Previous:
            // - ProcessGenesisBlock Timeline (4s) - Async
            // - ProcessGenesisBlock Timeline (19s) - Sync
            JobPoolWorker* worker = JobScheduler::GetRandomWorker();
            ProcessBlockJob* job = new ProcessBlockJob(genesis);
            worker->Submit(job);
            worker->Wait(job);
        }

        LOG(INFO) << "block chain initialized!";
        BlockChain::SetState(BlockChain::kInitialized);
        return true;
    }

    BlockChain::State BlockChain::GetState(){
        LOCK_GUARD;
        return state_;
    }

    void BlockChain::SetState(State state){
        LOCK_GUARD;
        state_ = state;
        SIGNAL_ALL;
    }

    BlockChain::Status BlockChain::GetStatus(){
        LOCK_GUARD;
        return status_;
    }

    void BlockChain::SetStatus(Status status){
        LOCK_GUARD;
        status_ = status;
        SIGNAL_ALL;
    }

    BlockPtr BlockChain::GetGenesis(){
        LOCK_GUARD;
        return GetBlock(GetReference(BLOCKCHAIN_REFERENCE_GENESIS));
    }

    BlockPtr BlockChain::GetHead(){
        return GetBlock(GetReference(BLOCKCHAIN_REFERENCE_HEAD));
    }

    BlockPtr BlockChain::GetBlock(const Hash& hash){
        leveldb::ReadOptions options;
        std::string key = hash.HexString();
        std::string filename;

        LOCK_GUARD;
        leveldb::Status status = GetIndex()->Get(options, key, &filename);
        if(!status.ok()){
            LOG(WARNING) << "couldn't find block " << hash << " in index: " << status.ToString();
            return nullptr;
        }

        BlockFileReader reader(filename);
        BlockPtr blk = reader.Read();
        if(hash != blk->GetHash()){
            LOG(WARNING) << "couldn't match block hashes: " << hash << " <=> " << blk->GetHash();
            return nullptr;
        }
        return blk;
    }

    BlockPtr BlockChain::GetBlock(int64_t height){
        leveldb::ReadOptions options;
        std::string key = GetBlockHeightKey(height);
        std::string filename;

        LOCK_GUARD;
        leveldb::Status status = GetIndex()->Get(options, key, &filename);
        if(!status.ok()){
            LOG(WARNING) << "couldn't find block #" << height << " in index: " << status.ToString();
            return nullptr;
        }

        BlockFileReader reader(filename);
        return reader.Read();
    }

    bool BlockChain::HasReference(const std::string& name){
        leveldb::ReadOptions options;
        std::string value;
        LOCK_GUARD;
        return GetIndex()->Get(options, name, &value).ok();
    }

    bool BlockChain::RemoveReference(const std::string& name){
        leveldb::WriteOptions options;
        std::string value;
        LOCK_GUARD;
        leveldb::Status status = GetIndex()->Delete(options, name);
        if(!status.ok()){
            LOG(WARNING) << "couldn't remove reference " << name << ": " << status.ToString();
            return false;
        }

        LOG(INFO) << "removed reference: " << name;
        return true;
    }

    bool BlockChain::PutReference(const std::string& name, const Hash& hash){
        leveldb::WriteOptions options;
        options.sync = true;

        LOCK_GUARD;
        leveldb::Status status = GetIndex()->Put(options, name, hash.HexString());
        if(!status.ok()){
            LOG(WARNING) << "couldn't set reference " << name << " to " << hash << ": " << status.ToString();
            return false;
        }

        LOG(INFO) << "set reference " << name << " := " << hash;
        return true;
    }

    Hash BlockChain::GetReference(const std::string& name){
        leveldb::ReadOptions options;
        std::string value;
        LOCK_GUARD;
        leveldb::Status status = GetIndex()->Get(options, name, &value);
        if(!status.ok()){
            LOG(WARNING) << "couldn't find reference " << name << ": " << status.ToString();
            return Hash();
        }
        return Hash::FromHexString(value);
    }

    static inline bool
    WriteToFile(const BlockPtr& blk, const std::string& filename){
        BlockFileWriter writer(filename);
        return writer.Write(blk);
    }

    bool BlockChain::PutBlock(const Hash& hash, BlockPtr blk){
        BlockHeader block = blk->GetHeader();

        leveldb::WriteOptions options;
        options.sync = true;
        std::string key = block.GetHash().HexString();
        std::string filename = GetNewBlockFilename(blk);

        LOCK_GUARD;
        if(FileExists(filename)){
            LOG(WARNING) << "couldn't overwrite existing block data: " << filename;
            return false;
        }

        if(!WriteToFile(blk, filename)){
            LOG(WARNING) << "couldn't write block " << block << " to file: " << filename;
            return false;
        }

        leveldb::Status status = GetIndex()->Put(options, key, filename);
        if(!status.ok()){
            LOG(WARNING) << "couldn't index block " << block << " by hash: " << status.ToString();
            return false;
        }

        status = GetIndex()->Put(options, GetBlockHeightKey(blk->GetHeight()), filename);
        if(!status.ok()){
            LOG(WARNING) << "couldn't index block " << block << " by height: " << status.ToString();
            return false;
        }

        LOG(INFO) << "indexed block " << block << " to file: " << filename;
        return true;
    }

    bool BlockChain::HasBlock(const Hash& hash){
        leveldb::ReadOptions options;
        std::string key = hash.HexString();
        std::string filename;

        LOCK_GUARD;
        leveldb::Status status = GetIndex()->Get(options, key, &filename);
        if(!status.ok()){
            LOG(WARNING) << "couldn't find block " << hash << ": " << status.ToString();
            return false;
        }
        return true;
    }

    bool BlockChain::Append(const BlockPtr& block){
        LOCK_GUARD;
        BlockPtr head = GetHead();
        Hash hash = block->GetHash();
        Hash phash = block->GetPreviousHash();

        LOG(INFO) << "appending new block:";
        LOG(INFO) << "  - Parent Hash: " << phash;
        LOG(INFO) << "  - Hash: " << hash;
        if(HasBlock(hash)){
            LOG(WARNING) << "duplicate block found for: " << hash;
            return false;
        }

        if(block->IsGenesis()){
            LOG(WARNING) << "cannot append genesis block: " << hash;
            return false;
        }

        if(!HasBlock(phash)){
            LOG(WARNING) << "cannot find parent block: " << phash;
            return false;
        }

        PutBlock(hash, block);
        if(head->GetHeight() < block->GetHeight())
            PutReference(BLOCKCHAIN_REFERENCE_HEAD, hash);
        return true;
    }

    bool BlockChain::VisitBlocks(BlockChainBlockVisitor* vis){
        Hash current = GetReference(BLOCKCHAIN_REFERENCE_HEAD);
        do{
            BlockPtr blk = GetBlock(current);
            if(!vis->Visit(blk))
                return false;
            current = blk->GetPreviousHash();
        } while(!current.IsNull());
        return true;
    }

    bool BlockChain::GetBlocks(HashList& hashes){
        Hash current = GetReference(BLOCKCHAIN_REFERENCE_HEAD);
        do{
            BlockPtr blk = GetBlock(current);
            hashes.insert(blk->GetHash());
            current = blk->GetPreviousHash();
        } while(!current.IsNull());
        return true;
    }

    bool BlockChain::VisitHeaders(BlockChainHeaderVisitor* vis){
        //TODO: Optimize BlockChain::VisitHeaders
        Hash current = GetReference(BLOCKCHAIN_REFERENCE_HEAD);
        do{
            BlockPtr blk = GetBlock(current);
            if(!vis->Visit(blk->GetHeader()))
                return false;
            current = blk->GetPreviousHash();
        } while(!current.IsNull());
        return true;
    }

    int64_t BlockChain::GetNumberOfBlocks(){
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
            LOG(WARNING) << "couldn't list files in block chain index: " << GetDataDirectory();
            return 0;
        }
        return count;
    }
}