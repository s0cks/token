#include <glog/logging.h>
#include "block_chain.h"
#include "block.h"

namespace Token{
    bool Block::GetBytes(CryptoPP::SecByteBlock& bytes) const{
        Proto::BlockChain::Block raw;
        raw << (*this);
        bytes.resize(raw.ByteSizeLong());
        return raw.SerializeToArray(bytes.data(), bytes.size());
    }

    bool Block::Contains(const uint256_t& hash) const{
        MerkleTreeBuilder builder(this);
        if(!builder.BuildTree()) return false;
        MerkleTree* tree = builder.GetMerkleTree();
        std::vector<MerkleProofHash> trail;
        if(!tree->BuildAuditProof(hash, trail)) return false;
        return tree->VerifyAuditProof(tree->GetMerkleRootHash(), hash, trail);
    }

    MerkleTree* Block::GetMerkleTree() const{
        MerkleTreeBuilder builder(this);
        if(!builder.BuildTree()) return nullptr;
        return builder.GetMerkleTree();
    }

    uint256_t Block::GetMerkleRoot() const{
        MerkleTreeBuilder builder(this);
        if(!builder.BuildTree()) return uint256_t();
        return builder.GetMerkleTree()->GetMerkleRootHash();
    }

    bool Block::Accept(BlockVisitor* vis) const{
        if(!vis->VisitBlockStart()) return false;
        for(size_t idx = 0;
            idx < GetNumberOfTransactions();
            idx++){
            Transaction* tx;
            if(!(tx = GetTransaction(idx))) return false;
            if(!vis->VisitTransaction(tx)){
                return false;
            }
            delete tx;
        }
        return vis->VisitBlockEnd();
    }

    bool MerkleTreeBuilder::VisitTransaction(Transaction* tx){
        AddLeaf((*tx));
        return true;
    }

    bool MerkleTreeBuilder::BuildTree(){
        if(HasTree()) return false;
        if(!GetBlock()->Accept(this)) return false;
        tree_ = new MerkleTree(leaves_);
        return true;
    }
}