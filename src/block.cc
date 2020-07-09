#include "block.h"
#include "block_chain.h"
#include "node/node.h"
#include "crash_report.h"

namespace Token{
//######################################################################################################################
//                                          Block Header
//######################################################################################################################
    BlockHeader::BlockHeader(Block* blk): BlockHeader(blk->GetTimestamp(), blk->GetHeight(), blk->GetPreviousHash(), blk->GetMerkleRoot(),
                                                      blk->GetSHA256Hash()){}

    Block* BlockHeader::GetData() const{
        return BlockChain::GetBlockData(GetHash());
    }

//######################################################################################################################
//                                          Block
//######################################################################################################################
    Block* Block::NewInstance(uint32_t height, const Token::uint256_t &phash, std::vector<Transaction*>& transactions,
                              uint32_t timestamp){
        Block* instance = (Block*)Allocator::Allocate(sizeof(Block), Object::kBlock);
        new (instance)Block(timestamp, height, phash, transactions);
        for(size_t idx = 0; idx < transactions.size(); idx++) Allocator::AddStrongReference(instance, transactions[idx], NULL);
        return instance;
    }

    Block* Block::NewInstance(const BlockHeader &parent, std::vector<Transaction *> &transactions, uint32_t timestamp){
        return NewInstance(parent.GetHeight() + 1, parent.GetHash(), transactions, timestamp);
    }

    Block* Block::NewInstance(RawType raw){
        Block* instance = (Block*)Allocator::Allocate(sizeof(Block), Object::kBlock);

        std::vector<Transaction*> txs = {};
        for(auto& it : raw.transactions()){
            txs.push_back(Transaction::NewInstance(it));
        }

        new (instance)Block(raw.timestamp(), raw.height(), HashFromHexString(raw.previous_hash()), txs);
        return instance;
    }

    Block* Block::NewInstance(std::fstream& fd){
        Block::RawType raw;
        if(!raw.ParseFromIstream(&fd)) return nullptr;
        return NewInstance(raw);
    }

    std::string Block::ToString() const{
        std::stringstream stream;
        stream << "Block(#" << GetHeight() << "/" << GetSHA256Hash() << ")";
        return stream.str();
    }

    bool Block::Encode(Block::RawType& raw) const{
        raw.set_timestamp(timestamp_);
        raw.set_height(height_);
        raw.set_previous_hash(HexString(previous_hash_));
        raw.set_merkle_root(HexString(GetMerkleRoot()));
        for(auto& it : transactions_){
            Transaction::RawType* raw_tx = raw.add_transactions();
            it->Encode((*raw_tx));
        }
        return true;
    }

    bool Block::Accept(BlockVisitor* vis) const{
        if(!vis->VisitStart()) return false;
        for(size_t idx = 0;
            idx < GetNumberOfTransactions();
            idx++){
            Transaction* tx = GetTransaction(idx);
            if(!vis->Visit(tx)) return false;
        }
        return vis->VisitEnd();
    }

    class BlockMerkleTreeBuilder : public BlockVisitor,
                                   public MerkleTreeBuilder{
    private:
        const Block* block_;
    public:
        BlockMerkleTreeBuilder(const Block* block):
                BlockVisitor(),
                MerkleTreeBuilder(),
                block_(block){}
        ~BlockMerkleTreeBuilder(){}

        const Block* GetBlock() const{
            return block_;
        }

        bool Visit(Transaction* tx){
            return AddLeaf(tx->GetSHA256Hash());
        }

        bool BuildTree(){
            if(HasTree()) return false;
            if(!GetBlock()->Accept(this)) return false;
            return CreateTree();
        }
    };

    bool Block::Finalize(){
        for(auto& it : transactions_) Allocator::RemoveStrongReference(this, it);
        return true;
    }

    bool Block::Contains(const uint256_t& hash) const{
        BlockMerkleTreeBuilder builder(this);
        if(!builder.BuildTree()) return false;
        MerkleTree* tree = builder.GetTree();
        std::vector<MerkleProofHash> trail;
        if(!tree->BuildAuditProof(hash, trail)) return false;
        return tree->VerifyAuditProof(tree->GetMerkleRootHash(), hash, trail);
    }

    uint256_t Block::GetMerkleRoot() const{
        //TODO: scope merkle tree
        BlockMerkleTreeBuilder builder(this);
        if(!builder.BuildTree()) return uint256_t();
        MerkleTree* tree = builder.GetTree();
        return tree->GetMerkleRootHash();
    }

//######################################################################################################################
//                                          Block Pool
//######################################################################################################################
    static std::recursive_mutex mutex_;
    static leveldb::DB* index_ = nullptr;
    static BlockPool::State state_ = BlockPool::kUninitialized;

    static inline std::string
    GetDataDirectory(){
        return FLAGS_path + "/blocks";
    }

    static inline leveldb::DB*
    GetIndex(){
        return index_;
    }

    static inline std::string
    GetNewBlockFilename(const uint256_t& hash){
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
    GetBlockFilename(const uint256_t& hash){
        leveldb::ReadOptions options;
        std::string key = HexString(hash);
        std::string filename;
        if(!GetIndex()->Get(options, key, &filename).ok()) return GetNewBlockFilename(hash);
        return filename;
    }

#define LOCK_GUARD std::lock_guard<std::recursive_mutex> guard(mutex_);

    void BlockPool::SetState(BlockPool::State state){
        LOCK_GUARD;
        state_ = state;
    }

    BlockPool::State BlockPool::GetState(){
        LOCK_GUARD;
        return state_;
    }

    void BlockPool::Initialize(){
        if(!IsUninitialized()){
            std::stringstream ss;
            ss << "Cannot reinitialize the block pool";
            CrashReport::GenerateAndExit(ss);
        }

        SetState(State::kInitializing);
        std::string filename = GetDataDirectory();
        if(!FileExists(filename)){
            if(!CreateDirectory(filename)){
                std::stringstream ss;
                ss << "cannot create BlockPool data directory: " << filename;
                CrashReport::GenerateAndExit(ss);
            }
        }

        leveldb::Options options;
        options.create_if_missing = true;
        if(!leveldb::DB::Open(options, (filename + "/index"), &index_).ok()){
            std::stringstream ss;
            ss << "Cannot create block pool index: " << (filename + "/index");
            CrashReport::GenerateAndExit(ss);
        }

        SetState(State::kInitialized);
#ifdef TOKEN_DEBUG
        LOG(INFO) << "initialized the block pool";
#endif//TOKEN_DEBUG
    }

    void BlockPool::RemoveBlock(const uint256_t& hash){
        if(!HasBlock(hash)) return;

        LOCK_GUARD;
        std::string filename = GetBlockFilename(hash);
        if(!DeleteFile(filename)){
            std::stringstream ss;
            ss << "Couldn't remove block " << hash << " data file: " << filename;
            CrashReport::GenerateAndExit(ss);
        }

        leveldb::WriteOptions options;
        if(!GetIndex()->Delete(options, HexString(hash)).ok()){
            std::stringstream ss;
            ss << "Couldn't remove block " << hash << " from index";
            CrashReport::GenerateAndExit(ss);
        }
    }

    void BlockPool::PutBlock(Block* block){
        uint256_t hash = block->GetSHA256Hash();
        if(HasBlock(hash)){
            std::stringstream ss;
            ss << "Couldn't add duplicate block " << hash << " to pool";
            CrashReport::GenerateAndExit(ss);
        }

        LOCK_GUARD;
        std::string filename = GetNewBlockFilename(hash);
        std::fstream fd(filename, std::ios::out|std::ios::binary);
        if(!block->WriteToFile(fd)){
            std::stringstream ss;
            ss << "Couldnt't write block " << hash << " to file: " << filename;
            CrashReport::GenerateAndExit(ss);
        }

        leveldb::WriteOptions options;
        std::string key = HexString(hash);
        if(!GetIndex()->Put(options, key, filename).ok()){
            std::stringstream ss;
            ss << "Couldn't index block: " << hash;
            CrashReport::GenerateAndExit(ss);
        }
    }

    bool BlockPool::HasBlock(const uint256_t& hash){
        leveldb::ReadOptions options;
        std::string key = HexString(hash);
        std::string filename;
        LOCK_GUARD;
        return GetIndex()->Get(options, key, &filename).ok();
    }

    bool BlockPool::Accept(BlockPoolVisitor* vis){
        CrashReport::GenerateAndExit("Need to implement");
    }

    bool BlockPool::GetBlocks(std::vector<uint256_t>& blocks){
        LOCK_GUARD;
        DIR* dir;
        struct dirent* ent;
        if((dir = opendir(GetDataDirectory().c_str())) != NULL){
            while((ent = readdir(dir)) != NULL){
                std::string name(ent->d_name);
                std::string filename = GetDataDirectory() + "/" + name;
                if(!EndsWith(filename, ".dat")) continue;
                Block* block = Block::NewInstance(filename);
                blocks.push_back(block->GetSHA256Hash());
            }
            closedir(dir);
            return true;
        }
        return false;
    }

    Block* BlockPool::GetBlock(const uint256_t& hash){
        if(!HasBlock(hash)) return nullptr;
        LOCK_GUARD;
        std::string filename = GetBlockFilename(hash);
        return Block::NewInstance(filename);
    }
}