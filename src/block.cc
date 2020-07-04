#include "block_chain.h"
#include "node/node.h"

namespace Token{
//######################################################################################################################
//                                          Block Header
//######################################################################################################################
    BlockHeader::BlockHeader(Block* blk): BlockHeader(blk->GetTimestamp(), blk->GetHeight(), blk->GetPreviousHash(), blk->GetMerkleRoot(), blk->GetHash()){}

    Block* BlockHeader::GetData() const{
        return BlockChain::GetBlockData(GetHash());
    }

//######################################################################################################################
//                                          Block
//######################################################################################################################
    Block* Block::NewInstance(uint32_t height, const Token::uint256_t &phash, std::vector<Transaction*>& transactions,
                              uint32_t timestamp){
        Block* instance = (Block*)Allocator::Allocate(sizeof(Block));
        new (instance)Block(timestamp, height, phash, transactions);
        for(size_t idx = 0; idx < transactions.size(); idx++){
            if(!Allocator::AddStrongReference(instance, transactions[idx], NULL)){
                CrashReport::GenerateAndExit("Couldn't add strong reference to transaction");
            }
        }
        return instance;
    }

    Block* Block::NewInstance(const BlockHeader &parent, std::vector<Transaction *> &transactions, uint32_t timestamp){
        return NewInstance(parent.GetHeight() + 1, parent.GetHash(), transactions, timestamp);
    }

    Block* Block::NewInstance(const Block::RawType& raw){
        Block* instance = (Block*)Allocator::Allocate(sizeof(Block));
        Allocator::AddRoot(instance);
        std::vector<Transaction*> txs = {};
        for(auto& it : raw.transactions()){
            Transaction* tx = (Transaction::NewInstance(it));
            if(!Allocator::AddStrongReference(instance, tx, NULL)){
                CrashReport::GenerateAndExit("Couldn't add strong reference to transaction");
            }
        }
        new (instance)Block(raw.timestamp(), raw.height(), HashFromHexString(raw.previous_hash()), txs);
        Allocator::RemoveRoot(instance);
        return instance;
    }

    Block* Block::NewInstance(std::fstream& fd){
        RawType raw;
        if(!raw.ParseFromIstream(&fd)) return nullptr;
        return NewInstance(raw);
    }

    std::string Block::ToString() const{
        std::stringstream stream;
        stream << "Block(#" << GetHeight() << "/" << GetHash() << ")";
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
            return AddLeaf(tx->GetHash());
        }

        bool BuildTree(){
            if(HasTree()) return false;
            if(!GetBlock()->Accept(this)) return false;
            return CreateTree();
        }
    };

    bool Block::Finalize(){
        for(auto& it : transactions_){
            if(!Allocator::RemoveStrongReference(this, it)){
                CrashReport::GenerateAndExit("Couldn't remove strong reference to transaction");
            }
        }
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

    MerkleTree* Block::GetMerkleTree() const{
        BlockMerkleTreeBuilder builder(this);
        if(!builder.BuildTree()) return nullptr;
        //TODO: return builder.GetTreeCopy();
        return nullptr;
    }

    uint256_t Block::GetMerkleRoot() const{
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
#ifdef TOKEN_ENABLE_DEBUG
        LOG(INFO) << "initialized the block pool";
#endif//TOKEN_ENABLE_DEBUG
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

#ifdef TOKEN_ENABLE_DEBUG
        LOG(INFO) << "removed block " << hash << " from block pool";
#endif//TOKEN_ENABLE_DEBUG
    }

    void BlockPool::PutBlock(Block* block){
        uint256_t hash = block->GetHash();
        if(HasBlock(hash)){
            std::stringstream ss;
            ss << "Couldn't add duplicate block " << hash << " to pool";
            CrashReport::GenerateAndExit(ss);
        }

        std::string filename = GetNewBlockFilename(hash);
        std::fstream fd(filename, std::ios::out|std::ios::binary);
        if(!block->WriteToFile(fd)){
            std::stringstream ss;
            ss << "Couldnt't write block " << hash << " to file: " << filename;
            CrashReport::GenerateAndExit(ss);
        }

        LOCK_GUARD;
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
                blocks.push_back(block->GetHash());
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