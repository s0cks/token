#include "block_chain.h"

namespace Token{
    BlockHeader::BlockHeader(const Block& block):
            timestamp_(block.GetTimestamp()),
            height_(block.GetHeight()),
            previous_hash_(block.GetPreviousHash()),
            hash_(block.GetHash()),
            merkle_root_(block.GetMerkleRoot()){}

    Block* BlockHeader::GetData() const{
        return BlockChain::GetBlockData(GetHash());
    }

    bool Block::GetBytes(CryptoPP::SecByteBlock& bytes) const{
        Proto::BlockChain::Block raw;
        raw << (*this);
        bytes.resize(raw.ByteSizeLong());
        return raw.SerializeToArray(bytes.data(), bytes.size());
    }

    bool Block::Accept(BlockVisitor* vis) const{
        if(!vis->VisitStart()) return false;
        for(size_t idx = 0;
            idx < GetNumberOfTransactions();
            idx++){
            Transaction tx;
            if(!GetTransaction(idx, &tx)) return false;
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

        bool Visit(const Transaction& tx){
            return AddLeaf(tx.GetHash());
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
}