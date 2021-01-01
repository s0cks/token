#ifndef TOKEN_INCLUDE_MERKLE_NODE_H
#define TOKEN_INCLUDE_MERKLE_NODE_H

#include "hash.h"
#include "object.h"

namespace Token{
  class MerkleNode : public Object{
   protected:
    MerkleNode* parent_;
    MerkleNode* lchild_;
    MerkleNode* rchild_;
    Hash hash_;

    void SetParent(MerkleNode* node){
      parent_ = node;
    }

    void SetLeft(MerkleNode* node){
      lchild_ = node;
      if(node)
        node->SetParent(this);
    }

    void SetRight(MerkleNode* node){
      rchild_ = node;
      if(node)
        node->SetParent(this);
    }

    Hash ComputeHash() const{
      if(IsLeaf()) return hash_;
      Hash left = GetLeft()->GetHash();
      Hash right = GetRight()->GetHash();
      return Hash::Concat(left, right);
    }
   public:
    MerkleNode(const Hash& hash):
      parent_(),
      lchild_(),
      rchild_(),
      hash_(hash){}
    MerkleNode(MerkleNode* lchild, MerkleNode* rchild):
      parent_(nullptr),
      lchild_(nullptr),
      rchild_(nullptr),
      hash_(Hash::Concat(lchild->GetHash(), rchild->GetHash())){
      SetLeft(lchild);
      SetRight(rchild);
    }
    ~MerkleNode() = default;

    MerkleNode* GetParent() const{
      return parent_;
    }

    MerkleNode* GetLeft() const{
      return lchild_;
    }

    bool HasLeft() const{
      return lchild_ != nullptr;
    }

    MerkleNode* GetRight() const{
      return rchild_;
    }

    bool HasRight() const{
      return rchild_ != nullptr;
    }

    bool IsLeaf() const{
      return lchild_ == nullptr
        && rchild_ == nullptr;
    }

    Hash GetHash() const{
      return hash_;
    }

    bool Equals(MerkleNode* node) const{
      return hash_ == node->hash_;
    }

    bool CanVerifyHash() const{
      return HasLeft() && HasRight();
    }

    bool VerifyHash() const{
      if(IsLeaf()) return true;
      if(!HasRight()) return GetHash() == GetLeft()->GetHash();
      return GetHash() == ComputeHash();
    }

    int GetLeaves() const{
      if(IsLeaf()) return 1;
      return GetLeft()->GetLeaves() + GetRight()->GetLeaves();
    }

    std::string ToString() const{
      std::stringstream ss;
      ss << "MerkleNode(" << hash_ << ")";
      return ss.str();
    }
  };
}

#endif //TOKEN_INCLUDE_MERKLE_NODE_H