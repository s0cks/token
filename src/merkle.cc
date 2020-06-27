#include <glog/logging.h>
#include "merkle.h"

namespace Token{
    MerkleNode* MerkleNode::NewInstance(const uint256_t& hash){
        MerkleNode* instance = (MerkleNode*)Allocator::Allocate(sizeof(MerkleNode));
        new (instance)MerkleNode(hash);
        return instance;
    }

    MerkleNode* MerkleNode::NewInstance(MerkleNode* left, MerkleNode* right){
        MerkleNode* instance = (MerkleNode*)Allocator::Allocate(sizeof(MerkleNode));
        new (instance)MerkleNode(left, right);
        return instance;
    }

    MerkleNode* MerkleNode::Clone(MerkleNode* node){
        MerkleNode* instance = (MerkleNode*)Allocator::Allocate(sizeof(MerkleNode));
        new (instance)MerkleNode(node);
        return instance;
    }

    std::string MerkleNode::ToString() const{
        std::stringstream ss;
        ss << "MerkleNode(" << hash_ << ")";
        return ss.str();
    }

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
        std::vector<MerkleNode*> nodes;
        for(auto& it : leaves){
            MerkleNode* node = MerkleNode::NewInstance(it);
            leaves_.push_back(node);
            nodes_.insert(std::make_pair(it, node));
            nodes.push_back(node);
        }
        size_t height = std::ceil(log2(leaves.size())) + 1;
        return BuildMerkleTree(height, nodes);
    }

    MerkleNode* MerkleTree::BuildMerkleTree(size_t height, std::vector<MerkleNode*>& nodes){
        if(height == 1 && !nodes.empty()){
            MerkleNode* node = nodes.back();
            nodes.pop_back();
            uint256_t hash = node->GetHash();
            nodes_.insert(std::make_pair(hash, node));
            return node;
        } else if(height > 1){
            MerkleNode* lchild = BuildMerkleTree(height - 1, nodes);
            MerkleNode* rchild;
            if(nodes.empty()){
                rchild = MerkleNode::Clone(lchild);
            } else{
                rchild = BuildMerkleTree(height - 1, nodes);
            }

            uint256_t hash = ConcatHashes(lchild->GetHash(), rchild->GetHash());
            MerkleNode* node = MerkleNode::NewInstance(hash);
            node->SetLeft(lchild);
            node->SetRight(rchild);
            nodes_.insert(std::make_pair(hash, node));
            return node;
        }
        return nullptr;
    }

    MerkleTree* MerkleTree::NewInstance(std::vector<uint256_t>& leaves){
        MerkleTree* instance = (MerkleTree*)Allocator::Allocate(sizeof(MerkleTree));
        new (instance)MerkleTree(leaves);
        return instance;
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

    MerkleNode* MerkleTree::GetNode(const uint256_t& hash) const{
        auto pos = nodes_.find(hash);
        if(pos == nodes_.end()) return nullptr;
        return pos->second;
    }

    MerkleNode* MerkleTree::GetLeafNode(const uint256_t& hash) const{
        for(auto& it : leaves_) if(it->GetHash() == hash) return it;
        return nullptr;
    }

    bool MerkleTree::BuildAuditProof(const uint256_t& hash, std::vector<MerkleProofHash>& trail){
        MerkleNode* leaf = GetLeafNode(hash);
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
        uint64_t idx = log2l(m);

        MerkleNode* node = leaves_[0];
        while(idx > 0){
            node = node->GetParent();
            idx--;
        }

        int k = node->GetLeaves();
        LOG(INFO) << "node leaves: " << k;
        if(m != k){
            MerkleNode* sn = node->GetParent()->GetRight();
            while(true){
                int sncount = sn->GetLeaves();
                LOG(INFO) << "sibling node leaves: " << sncount;
                if(m - k == sncount){
                    LOG(INFO) << "adding: " << sn->GetHash();
                    trail.push_back(MerkleProofHash(MerkleProofHash::Direction::kRoot, sn->GetHash()));
                    break;
                }

                if(m - k > sncount){
                    LOG(INFO) << "adding: " << sn->GetHash();
                    trail.push_back(MerkleProofHash(MerkleProofHash::Direction::kRoot, sn->GetHash()));
                    sn = sn->GetParent()->GetRight();
                    k += sncount;
                } else{
                    sn = sn->GetLeft();
                }
            }
        }
        return true;
    }

    bool MerkleTree::BuildConsistencyAuditProof(const uint256_t& hash, std::vector<MerkleProofHash>& trail){
        MerkleNode* node = GetNode(hash);
        MerkleNode* parent = node->GetParent();
        BuildAuditTrail(trail, parent, node);
        return true;
    }

    bool MerkleTree::VerifyConsistencyProof(const uint256_t& root, std::vector<MerkleProofHash>& proof){
        uint256_t hash, lhash, rhash;
        if(proof.size() > 1){
            lhash = proof[proof.size() - 2].GetHash();
            int hidx = proof.size() - 1;
            hash = rhash = ConcatHashes(lhash, proof[hidx].GetHash());
            hidx -= 2;
            while(hidx >= 0){
                lhash = proof[hidx].GetHash();
                hash = rhash = ConcatHashes(lhash, rhash);
                hidx--;
            }
        } else{
            hash = proof[0].GetHash();
        }
        return hash == root;
    }
}