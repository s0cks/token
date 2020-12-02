#include "block.h"
#include "server.h"
#include "block_chain.h"
#include "crash_report.h"

namespace Token{
//######################################################################################################################
//                                          Block Header
//######################################################################################################################
    BlockHeader::BlockHeader(Block* blk):
        timestamp_(blk->timestamp_),
        height_(blk->height_),
        previous_hash_(blk->previous_hash_),
        merkle_root_(blk->GetMerkleRoot()),
        hash_(blk->GetHash()),
        bloom_(blk->tx_bloom_){}

    BlockHeader::BlockHeader(Buffer* buff):
        timestamp_(buff->GetLong()),
        height_(buff->GetLong()),
        previous_hash_(buff->GetHash()),
        merkle_root_(buff->GetHash()),
        hash_(buff->GetHash()),
        bloom_(){
    }

    Block* BlockHeader::GetData() const{
        return BlockChain::GetBlock(GetHash());
    }

    bool BlockHeader::Write(Buffer* buff) const{
        buff->PutLong(timestamp_);
        buff->PutLong(height_);
        buff->PutHash(previous_hash_);
        buff->PutHash(merkle_root_);
        buff->PutHash(hash_);
        return true;
    }

//######################################################################################################################
//                                          Block
//######################################################################################################################
    Block* Block::Genesis(){
        InputList inputs;

        int64_t idx;

        OutputList outputs_a;
        for(idx = 0;
            idx < Block::kNumberOfGenesisOutputs;
            idx++){
            std::string user = "VenueA";
            std::stringstream ss;
            ss << "TestToken" << idx;
            outputs_a.push_back(Output(user, ss.str()));
        }

        OutputList outputs_b;
        for(idx = 0;
            idx < Block::kNumberOfGenesisOutputs;
            idx++){
            std::string user = "VenueB";
            std::stringstream ss;
            ss << "TestToken" << idx;
            outputs_b.push_back(Output(user, ss.str()));
        }

        OutputList outputs_c;
        for(idx = 0;
            idx < Block::kNumberOfGenesisOutputs;
            idx++){
            std::string user = "VenueC";
            std::stringstream ss;
            ss << "TestToken" << idx;
            outputs_c.push_back(Output(user, ss.str()));
        }

        TransactionList transactions = {
            Transaction(0, inputs, outputs_a, 0),
            Transaction(1, inputs, outputs_b, 0),
            Transaction(2, inputs, outputs_c, 0),
        };
        return new Block(0, Hash(), transactions, 0);
    }

    Block* Block::NewInstance(std::fstream& fd, int64_t size){
        Buffer buff(size);
        buff.ReadBytesFrom(fd, size);
        return NewInstance(&buff);
    }

    Block* Block::NewInstance(Buffer* buff){
        Timestamp timestamp = buff->GetLong();
        intptr_t height = buff->GetLong();
        Hash phash = buff->GetHash();
        intptr_t num_txs = buff->GetLong();

        TransactionList transactions;
        for(int64_t idx = 0;
            idx < num_txs;
            idx++){
            transactions.push_back(Transaction(buff));
        }
        return new Block(height, phash, transactions, timestamp);
    }

    bool Block::ToJson(Json::Value& value) const{
        value["timestamp"] = GetTimestamp();
        value["height"] = GetHeight();
        value["previous_hash"] = GetPreviousHash().HexString();
        value["merkle_root"] = GetMerkleRoot().HexString();
        value["hash"] = GetHash().HexString();
        return true;
    }

    bool Block::Accept(BlockVisitor* vis) const{
        if(!vis->VisitStart())
            return false;

        int64_t idx;
        for(idx = 0;
            idx < GetNumberOfTransactions();
            idx++){
            if(!vis->Visit(transactions_[idx]))
                return false;
        }

        return vis->VisitEnd();
    }

    bool Block::Contains(const Hash& hash) const{
        return tx_bloom_.Contains(hash);
    }

    Hash Block::GetMerkleRoot() const{
        std::vector<Hash> hashes;
        for(auto& it : transactions_)
            hashes.push_back(it.GetHash());
        MerkleTree tree(hashes);
        return tree.GetMerkleRootHash();
    }

    static std::mutex mutex_;
    static std::condition_variable cond_;

    static BlockPool::State state_ = BlockPool::kUninitialized;
    static BlockPool::Status status_ = BlockPool::kOk;
    static leveldb::DB* index_ = nullptr;

#define POOL_OK_STATUS(Status, Message) \
    LOG(INFO) << (Message);             \
    SetStatus((Status));
#define POOL_OK(Message) \
    POOL_OK_STATUS(BlockPool::kOk, (Message));
#define POOL_WARNING_STATUS(Status, Message) \
    LOG(WARNING) << (Message);               \
    SetStatus((Status));
#define POOL_WARNING(Message) \
    POOL_WARNING_STATUS(BlockPool::kWarning, (Message))
#define POOL_ERROR_STATUS(Status, Message) \
    LOG(ERROR) << (Message);               \
    SetStatus((Status));
#define POOL_ERROR(Message) \
    POOL_ERROR_STATUS(BlockPool::kError, (Message))
#define LOCK_GUARD std::lock_guard<std::mutex> guard(mutex_);
#define LOCK std::unique_lock<std::mutex> lock(mutex_)
#define UNLOCK lock.unlock()
#define WAIT cond_.wait(lock)
#define SIGNAL_ONE cond_.notify_one()
#define SIGNAL_ALL cond_.notify_all()

    static inline std::string
    GetDataDirectory(){
        return FLAGS_path + "/blocks";
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

    void BlockPool::SetState(const BlockPool::State& state){
        LOCK_GUARD;
        state_ = state;
    }

    BlockPool::State BlockPool::GetState(){
        LOCK_GUARD;
        return state_;
    }

    void BlockPool::SetStatus(const BlockPool::Status& status){
        LOCK_GUARD;
        status_ = status;
    }

    std::string BlockPool::GetStatusMessage(){
        std::stringstream ss;
        LOCK_GUARD;

        ss << "[";
        switch(state_){
#define DEFINE_STATE_MESSAGE(Name) \
            case BlockPool::k##Name: \
                ss << #Name; \
                break;
            FOR_EACH_UTXOPOOL_STATE(DEFINE_STATE_MESSAGE)
#undef DEFINE_STATE_MESSAGE
        }
        ss << "] ";

        switch(status_){
#define DEFINE_STATUS_MESSAGE(Name) \
            case BlockPool::k##Name:{ \
                ss << #Name; \
                break; \
            }
            FOR_EACH_UTXOPOOL_STATUS(DEFINE_STATUS_MESSAGE)
#undef DEFINE_STATUS_MESSAGE
        }
        return ss.str();
    }

    bool BlockPool::Initialize(){
        if(IsInitialized()){
            POOL_WARNING("cannot re-initialize the block pool.");
            return false;
        }

        LOG(INFO) << "initializing the block pool in: " << GetDataDirectory();
        SetState(BlockPool::kInitializing);
        if(!FileExists(GetDataDirectory())){
            if(!CreateDirectory(GetDataDirectory())){
                std::stringstream ss;
                ss << "couldn't create the block pool directory: " << GetDataDirectory();
                POOL_ERROR(ss.str());
                return false;
            }
        }

        leveldb::Options options;
        options.create_if_missing = true;
        if(!leveldb::DB::Open(options, GetIndexFilename(), &index_).ok()){
            std::stringstream ss;
            ss << "couldn't initialize the block pool index: " << GetIndexFilename();
            POOL_ERROR(ss.str());
            return false;
        }

        SetState(BlockPool::kInitialized);
        POOL_OK("block pool initialized.");
        return true;
    }

    bool BlockPool::HasBlock(const Hash& hash){
        leveldb::ReadOptions options;
        std::string key = hash.HexString();
        std::string filename;

        LOCK_GUARD;
        leveldb::Status status = GetIndex()->Get(options, key, &filename);
        if(!status.ok()){
            std::stringstream ss;
            ss << "couldn't find block " << hash << ": " << status.ToString();
            POOL_WARNING(ss.str());
            return false;
        }
        return true;
    }

    Block* BlockPool::GetBlock(const Hash& hash){
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

        Block* blk = Block::NewInstance(filename);
        if(hash != blk->GetHash()){
            std::stringstream ss;
            ss << "couldn't verify block hash: " << hash;
            POOL_WARNING(ss.str());
            //TODO: better validation + error handling for UnclaimedTransaction data
            return nullptr;
        }
        return blk;
    }

    bool BlockPool::RemoveBlock(const Hash& hash){
        if(!HasBlock(hash)){
            std::stringstream ss;
            ss << "cannot remove non-existing block: " << hash;
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
            ss << "couldn't remove block " << hash << " from pool: " << status.ToString();
            POOL_ERROR(ss.str());
            return false;
        }

        if(!DeleteFile(filename)){
            std::stringstream ss;
            ss << "couldn't remove the block file: " << filename;
            POOL_ERROR(ss.str());
            return false;
        }

        LOG(INFO) << "removed block: " << hash;
        return true;
    }

    bool BlockPool::PutBlock(const Hash& hash, Block* blk){
        leveldb::WriteOptions options;
        options.sync = true;
        std::string key = hash.HexString();
        std::string filename = GetNewDataFilename(hash);

        LOCK_GUARD;
        if(FileExists(filename)){
            std::stringstream ss;
            ss << "cannot overwrite existing block pool data: " << filename;
            POOL_ERROR(ss.str());
            return false;
        }

        if(!blk->WriteToFile(filename)){
            std::stringstream ss;
            ss << "cannot write block pool data to: " << filename;
            POOL_ERROR(ss.str());
            return false;
        }

        leveldb::Status status = GetIndex()->Put(options, key, filename);
        if(!status.ok()){
            std::stringstream ss;
            ss << "couldn't index block " << hash << ": " << status.ToString();
            POOL_ERROR(ss.str());
            return false;
        }

        LOG(INFO) << "added block to pool: " << hash;
        return true;
    }

    int64_t BlockPool::GetNumberOfBlocksInPool(){
        int64_t count = 0;
        LOCK_GUARD;
        DIR* dir;
        struct dirent* ent;
        if((dir = opendir(GetDataDirectory().c_str())) != NULL){
            while((ent = readdir(dir)) != NULL){
                std::string name(ent->d_name);
                std::string filename = (GetDataDirectory() + "/" + name);
                if(!EndsWith(filename, ".dat")) continue;
                count++;
            }
            closedir(dir);
        }
        return count;
    }

    bool BlockPool::GetBlocks(std::vector<Hash>& blocks){
        LOCK_GUARD;
        DIR* dir;
        struct dirent* ent;
        if((dir = opendir(GetDataDirectory().c_str())) != NULL){
            while((ent = readdir(dir)) != NULL){
                std::string name(ent->d_name);
                std::string filename = (GetDataDirectory() + "/" + name);
                if(!EndsWith(filename, ".dat")) continue;

                Block* blk = Block::NewInstance(filename);
                blocks.push_back(blk->GetHash());
            }
            closedir(dir);
            return true;
        }
        return false;
    }

    bool BlockPool::Accept(BlockPoolVisitor* vis){
        if(!vis->VisitStart()) return false;

        LOCK_GUARD;
        DIR* dir;
        struct dirent* ent;
        if((dir = opendir(GetDataDirectory().c_str())) != NULL){
            while((ent = readdir(dir)) != NULL){
                std::string name(ent->d_name);
                std::string filename = (GetDataDirectory() + "/" + name);
                if(!EndsWith(filename, ".dat")) continue;

                Block* blk = Block::NewInstance(filename);
                if(!vis->Visit(blk))
                    break;
            }
            closedir(dir);
        }

        return vis->VisitEnd();
    }

    void BlockPool::WaitForBlock(const Hash& hash){
        LOCK;
        while(!HasBlock(hash)) WAIT;
    }

    class BlockPoolPrinter : public BlockPoolVisitor{
    public:
        BlockPoolPrinter() = default;
        ~BlockPoolPrinter() = default;

        bool Visit(Block* blk){
            LOG(INFO) << blk->GetHash();
            return true;
        }
    };

    bool BlockPool::Print(){
        BlockPoolPrinter printer;
        return Accept(&printer);
    }
}