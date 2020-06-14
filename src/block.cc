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
        return instance;
    }

    Block* Block::NewInstance(const BlockHeader &parent, std::vector<Transaction *> &transactions, uint32_t timestamp){
        return NewInstance(parent.GetHeight() + 1, parent.GetHash(), transactions, timestamp);
    }

    Block* Block::NewInstance(const Block::RawType& raw){
        std::vector<Transaction*> txs = {};
        for(auto& it : raw.transactions()) txs.push_back(Transaction::NewInstance(it));
        return NewInstance(raw.height(), HashFromHexString(raw.previous_hash()), txs, raw.timestamp());
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
        return builder.GetTreeCopy();
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

#define READ_LOCK pthread_rwlock_tryrdlock(&pool->rwlock_)
#define WRITE_LOCK pthread_rwlock_trywrlock(&pool->rwlock_)
#define UNLOCK pthread_rwlock_unlock(&pool->rwlock_)

    BlockPool* BlockPool::GetInstance(){
        static BlockPool instance;
        return &instance;
    }

    bool BlockPool::Initialize(){
        BlockPool* pool = GetInstance();
        if(!FileExists(pool->GetRoot())){
            if(!CreateDirectory(pool->GetRoot())) return false;
        }
        return pool->InitializeIndex();
    }

    bool BlockPool::HasBlock(const uint256_t& hash){
        BlockPool* pool = GetInstance();
        READ_LOCK;
        bool found = pool->ContainsObject(hash);
        UNLOCK;
        return found;
    }

    Block* BlockPool::GetBlock(const uint256_t& hash){
        BlockPool* pool = GetInstance();
        READ_LOCK;
        Block* block = pool->LoadObject(hash);
        UNLOCK;
        return block;
    }

    bool BlockPool::RemoveBlock(const uint256_t& hash){
        BlockPool* pool = GetInstance();
        WRITE_LOCK;
        bool removed = pool->DeleteObject(hash);
        UNLOCK;
        return removed;
    }

    bool BlockPool::PutBlock(Block* block){
        BlockPool* pool = GetInstance();
        WRITE_LOCK;
        uint256_t hash = block->GetHash();
        bool written = pool->SaveObject(hash, block);
        UNLOCK;
        return written;
    }

    bool BlockPool::AddBlock(Block* block){
        //TODO: broadcast?
        BlockPool* pool = GetInstance();
        WRITE_LOCK;
        uint256_t hash = block->GetHash();
        bool written = pool->SaveObject(hash, block);
        Node::BroadcastInventory(block);
        UNLOCK;
        return written;
    }
}