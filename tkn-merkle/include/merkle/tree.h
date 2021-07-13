#ifndef TOKEN_INCLUDE_MERKLE_TREE_H
#define TOKEN_INCLUDE_MERKLE_TREE_H

#include <map>
#include <vector>
#include "node.h"
#include "proof.h"

namespace token{
  class MerkleTreeVisitor;
  class MerkleTree{
    friend class MerkleTreeBuilder;
   private:
    MerkleNode* root_;
    std::vector<Hash> leaves_;
    std::map<Hash, MerkleNode*> nodes_;

    void SetRoot(MerkleNode* node){
      root_ = node;
    }

    inline MerkleNode*
    CreateNode(const Hash& hash){
      MerkleNode* node = new MerkleNode(hash);
      nodes_.insert({hash, node});
      return node;
    }

    inline MerkleNode*
    CreateNode(MerkleNode* lchild, MerkleNode* rchild){
      MerkleNode* node = new MerkleNode(lchild, rchild);
      nodes_.insert({node->GetHash(), node});
      return node;
    }

    MerkleNode* BuildTree(std::vector<MerkleNode*>& nodes);
    MerkleNode* BuildTree(std::vector<Hash>& nodes);
   public:
    MerkleTree(const std::vector<Hash>& leaves):
      root_(nullptr),
      leaves_(leaves),
      nodes_(){
      SetRoot(BuildTree(leaves_));
    }
    ~MerkleTree() = default;

    MerkleNode* GetRoot() const{
      return root_;
    }

    Hash GetRootHash() const{
      return GetRoot()->GetHash();
    }

    int64_t GetNumberOfLeaves() const{
      return leaves_.size();
    }

    bool IsEmpty() const{
      return leaves_.empty();
    }

    bool GetLeaves(std::vector<Hash>& leaves) const{
      if(IsEmpty()){
        return false;
      }
      std::copy(leaves_.begin(), leaves_.end(), std::back_inserter(leaves));
      return leaves.size() == leaves_.size();
    }

    void Clear(){
      root_ = nullptr;
      nodes_.clear();
      leaves_.clear();
    }

    MerkleNode* GetNode(const Hash& hash) const;
    bool VisitLeaves(MerkleTreeVisitor* vis) const;
    bool VisitNodes(MerkleTreeVisitor* vis) const;
    bool Append(const std::unique_ptr<MerkleTree>& tree);
    bool BuildAuditProof(const Hash& hash, std::vector<MerkleProofHash>& trail);
    bool VerifyAuditProof(const Hash& root, const Hash& leaf, MerkleProof& trail);
    bool BuildConsistencyProof(intptr_t num_nodes, MerkleProof& trail);
    bool BuildConsistencyAuditProof(const Hash& hash, MerkleProof& trail);
    bool VerifyConsistencyProof(const Hash& root, MerkleProof& trail);

    inline bool VerifyAuditProof(const Hash& node, MerkleProof& proof){
      return VerifyAuditProof(GetRootHash(), node, proof);
    }

    static inline std::unique_ptr<MerkleTree>
    ConcatTrees(const std::unique_ptr<MerkleTree>& a, const std::unique_ptr<MerkleTree>& b){
      std::vector<Hash> leaves;
      std::copy(a->leaves_.begin(), a->leaves_.end(), std::back_inserter(leaves));
      std::copy(b->leaves_.begin(), b->leaves_.end(), std::back_inserter(leaves));
      return std::unique_ptr<MerkleTree>(new MerkleTree(leaves));
    }
  };

  typedef std::unique_ptr<MerkleTree> MerkleTreePtr;

  class MerkleTreeVisitor{
   protected:
    MerkleTreeVisitor() = default;
   public:
    virtual ~MerkleTreeVisitor() = default;
    virtual bool VisitStart() const{ return true; }
    virtual bool Visit(MerkleNode* node) const = 0;
    virtual bool VisitEnd() const{ return true; }
  };
}

#endif //TOKEN_INCLUDE_MERKLE_TREE_H