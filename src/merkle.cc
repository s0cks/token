#include <glog/logging.h>
#include "merkle.h"

namespace Token{
    bool MerkleNode::VerifyHash() const{
        if(IsLeaf()) return true;
        if(!HasRight()) return GetHash() == GetLeft()->GetHash();
        return GetHash() == ComputeHash();
    }

    uint256_t MerkleNode::ComputeHash() const{
        if(IsLeaf()) return hash_;
        uint256_t left = GetLeft()->GetHash();
        uint256_t right = GetRight()->GetHash();
        return ConcatHashes(left, right);
    }

    MerkleNode* MerkleTree::BuildMerkleTree(std::vector<uint256_t>& leaves){
        Array<MerkleNode*> nodes(leaves.size());
        for(auto& it : leaves){
            MerkleNode* node = new MerkleNode(it);
            nodes_.Add(node);
            leaves_.Add(node);
            nodes.Add(node);
        }
        size_t height = std::ceil(log2(leaves.size())) + 1;
        return BuildMerkleTree(height, nodes);
    }

    MerkleNode* MerkleTree::BuildMerkleTree(size_t height, Array<MerkleNode*>& nodes){
        if(height == 1 && !nodes.IsEmpty()){
            return nodes.Pop();
        } else if(height > 1){
            MerkleNode* lchild = BuildMerkleTree(height - 1, nodes);
            MerkleNode* rchild;
            if(nodes.IsEmpty()){
                rchild = new MerkleNode((*lchild));
            } else{
                rchild = BuildMerkleTree(height - 1, nodes);
            }

            MerkleNode* node = new MerkleNode(ConcatHashes(lchild->GetHash(), rchild->GetHash()));
            node->SetLeft(lchild);
            node->SetRight(rchild);
            nodes_.Add(node);
            return node;
        }
        return nullptr;
    }

    MerkleTree::MerkleTree(std::vector<uint256_t>& leaves):
        leaves_(),
        nodes_(),
        root_(nullptr){
        root_ = BuildMerkleTree(leaves);
    }

    bool MerkleTree::Append(const MerkleTree& tree){
        std::vector<uint256_t> leaves;
        if(!GetLeaves(leaves)) return false;
        if(!tree.GetLeaves(leaves)) return false;
        Clear();
        root_ = BuildMerkleTree(leaves);
        return true;
    }

    static void
    BuildAuditTrail(std::vector<MerkleProofHash>& trail, MerkleNode* parent, MerkleNode* child){
        if(parent){
            MerkleNode* next = parent->GetLeft()->Equals(child) ?
                    parent->GetRight() :
                    parent->GetLeft();
            MerkleProofHash::Direction direction = parent->GetLeft()->Equals(child) ?
                    MerkleProofHash::Direction::kLeft :
                    MerkleProofHash::Direction::kRight;

            if(next) trail.push_back(MerkleProofHash(direction, next->GetHash()));
            BuildAuditTrail(trail, child->GetParent()->GetParent(), child->GetParent());
        }
    }

    MerkleNode* MerkleTree::FindLeafNode(const uint256_t& hash){
        for(size_t idx = 0; idx < leaves_.Length(); idx++){
            if(leaves_[idx]->GetHash() == hash) return leaves_[idx];
        }
        return nullptr;
    }

    bool MerkleTree::BuildAuditProof(const uint256_t& hash, std::vector<MerkleProofHash>& trail){
        MerkleNode* leaf = FindLeafNode(hash);
        if(!leaf) return false;
        MerkleNode* parent = leaf->GetParent();
        BuildAuditTrail(trail, parent, leaf);
        return true;
    }

    bool MerkleTree::VerifyAuditProof(const uint256_t& root, const uint256_t& leaf, std::vector<MerkleProofHash>& trail){
        uint256_t testHash = leaf;
        for(auto& it : trail){
            testHash = it.IsLeft() ?
                    ConcatHashes(testHash, it.GetHash()) :
                    ConcatHashes(it.GetHash(), testHash);
        }
        return testHash == root;
    }

    bool MerkleTree::BuildConsistencyProof(uint64_t m, std::vector<MerkleProofHash>& trail){
        return false; //TODO: Implement
    }

    bool MerkleTree::VerifyConsistencyProof(const uint256_t& root, std::vector<MerkleProofHash>& proof){
        return false; //TODO: Implement
    }
}