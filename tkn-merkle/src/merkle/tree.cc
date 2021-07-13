#include "merkle/tree.h"

namespace token{
  MerkleNode* MerkleTree::BuildTree(std::vector<MerkleNode*>& nodes){
    if(nodes.size() == 1){
      return nodes.front();
    }

    std::vector<MerkleNode*> parents;
    for(size_t idx = 0; idx < nodes.size(); idx += 2){
      MerkleNode* lchild = nodes[idx];
      MerkleNode* rchild = nodes[idx + 1];
      parents.push_back(CreateNode(lchild, rchild));
    }
    return BuildTree(parents);
  }

  MerkleNode* MerkleTree::BuildTree(std::vector<Hash>& leaves){
    std::vector<MerkleNode*> nodes;
    for(auto& it : leaves)
      nodes.push_back(CreateNode(it));
    if((nodes.size() % 2) == 1){
      MerkleNode* node = nodes.back();
      nodes.push_back(node);
    }
    return BuildTree(nodes);
  }

  bool MerkleTree::VisitLeaves(MerkleTreeVisitor* vis) const{
    if(!vis->VisitStart()){
      return false;
    }
    for(auto& it : leaves_){
      MerkleNode* node;
      if(!(node = GetNode(it)) || !vis->Visit(node)){
        return false;
      }
    }
    return vis->VisitEnd();
  }

  bool MerkleTree::VisitNodes(MerkleTreeVisitor* vis) const{
    if(!vis->VisitStart()){
      return false;
    }
    for(auto& it : nodes_){
      if(!vis->Visit(it.second)){
        return false;
      }
    }
    return vis->VisitEnd();
  }

  bool MerkleTree::Append(const std::unique_ptr<MerkleTree>& tree){
    std::vector<Hash> leaves;
    std::copy(leaves_.begin(), leaves_.end(), std::back_inserter(leaves));
    std::copy(tree->leaves_.begin(), tree->leaves_.end(), std::back_inserter(leaves));
    Clear();
    leaves_ = leaves;
    root_ = BuildTree(leaves);
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

  MerkleNode* MerkleTree::GetNode(const Hash& hash) const{
    auto pos = nodes_.find(hash);
    if(pos == nodes_.end()){
      return nullptr;
    }
    return pos->second;
  }

  bool MerkleTree::BuildAuditProof(const Hash& hash, MerkleProof& trail){
    MerkleNode* node;
    if(!(node = GetNode(hash))){
      return false;
    }
    BuildAuditTrail(trail, node->GetParent(), node);
    return true;
  }

  bool MerkleTree::VerifyAuditProof(const Hash& root, const Hash& leaf, MerkleProof& trail){
    Hash testHash = leaf;
    for(auto& it : trail){
      testHash = it.IsLeft() ?
                 Hash::Concat(testHash, it.GetHash()) :
                 Hash::Concat(it.GetHash(), testHash);
    }
    return testHash == root;
  }

  bool MerkleTree::BuildConsistencyProof(int64_t m, MerkleProof& trail){
    int idx = (int) log2(m);

    MerkleNode* node = GetNode(leaves_.front());
    while(idx > 0){
      node = node->GetParent();
      idx--;
    }

    int k = node->GetLeaves();
    trail.push_back(MerkleProofHash(MerkleProofHash::kRoot, node->GetHash()));

    if(m != k){
      MerkleNode* sn = node->GetParent()->GetRight();
      while(true){
        int sncount = sn->GetLeaves();
        if(m - k == sncount){
          trail.push_back(MerkleProofHash(MerkleProofHash::kRoot, sn->GetHash()));
          break;
        }

        if(m - k > sncount){
          trail.push_back(MerkleProofHash(MerkleProofHash::kRoot, sn->GetHash()));
          sn = sn->GetParent()->GetRight();
          k += sncount;
        } else{
          sn = sn->GetLeft();
        }
      }
    }
    return true;
  }

  bool MerkleTree::BuildConsistencyAuditProof(const Hash& hash, MerkleProof& trail){
    MerkleNode* node = GetNode(hash);
    MerkleNode* parent = node->GetParent();
    BuildAuditTrail(trail, parent, node);
    return true;
  }

  bool MerkleTree::VerifyConsistencyProof(const Hash& root, MerkleProof& proof){
    Hash hash;
    Hash lhash;
    Hash rhash;
    if(proof.size() > 1){
      lhash = proof[proof.size() - 2].GetHash();
      int hidx = proof.size() - 1;
      hash = Hash::Concat(lhash, proof[hidx].GetHash());
      rhash = hash;
      hidx -= 2;
      while(hidx >= 0){
        lhash = proof[hidx].GetHash();
        hash = Hash::Concat(lhash, rhash);
        rhash = hash;
        hidx--;
      }
    } else{
      hash = proof.front().GetHash();
    }
    return hash == root;
  }
}