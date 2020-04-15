#include <glog/logging.h>
#include "block.h"
#include "block_chain.h"

namespace Token{
    BlockHeader::BlockHeader(const Block& block):
        timestamp_(block.GetTimestamp()),
        height_(block.GetHeight()),
        previous_hash_(block.GetPreviousHash()),
        hash_(block.GetHash()),
        merkle_root_(block.GetMerkleRoot()){}

    Block* BlockHeader::GetData() const{
        Block* result = new Block();
        if(!BlockChain::GetBlockData(GetHash(), result)) return nullptr;
        return result;
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
}