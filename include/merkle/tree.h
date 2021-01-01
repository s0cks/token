#ifndef TOKEN_INCLUDE_MERKLE_TREE_H
#define TOKEN_INCLUDE_MERKLE_TREE_H

#include <map>
#include <vector>
#include "block.h"
#include "merkle/node.h"
#include "transaction.h"
#include "merkle/proof.h"
#include "utils/printer.h"

namespace Token{
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
      if(IsEmpty())
        return false;
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

  class MerkleTreePrinter : public MerkleTreeVisitor, Printer{
   public:
    MerkleTreePrinter(Printer* parent):
      MerkleTreeVisitor(),
      Printer(parent){}
    MerkleTreePrinter(const google::LogSeverity& severity=google::INFO, const long& flags=Printer::kFlagNone):
      MerkleTreeVisitor(),
      Printer(severity, flags){}
    ~MerkleTreePrinter() = default;

    bool Visit(MerkleNode* node) const{
      LOG_AT_LEVEL(GetSeverity()) << node->GetHash();
      return true;
    }
  };

  class MerkleTreeBuilder : public BlockVisitor{
   private:
    std::vector<Hash> leaves_;
   public:
    MerkleTreePtr Build(){
      return std::unique_ptr<MerkleTree>(new MerkleTree(leaves_));
    }

    bool Visit(const TransactionPtr& tx){
      leaves_.push_back(tx->GetHash());
      return true;
    }

    static inline MerkleTreePtr
    Build(const BlockPtr& blk){
      MerkleTreeBuilder builder;
      if(!blk->Accept(&builder)){
        LOG(ERROR) << "couldn't build merkle tree for block";
        return nullptr;
      }
      return builder.Build();
    }
  };
}

#endif //TOKEN_INCLUDE_MERKLE_TREE_H