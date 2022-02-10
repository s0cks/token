#ifndef TOKEN_MERKLE_H
#define TOKEN_MERKLE_H

#include "token/hash.h"

namespace token::merkle{
 class Node;
 class NodeVisitor{
  protected:
   NodeVisitor() = default;
  public:
   virtual ~NodeVisitor() = default;
   virtual void Visit(const Node& node) const = 0;
 };

 class Node{
  protected:
   Node* parent_;
   Node* lchild_;
   Node* rchild_;
   uint256 hash_;

   inline void
   SetParent(Node* node){
     parent_ = node;
   }

   inline void
   SetLeft(Node* node){
     lchild_ = node;
     if(node)
       node->SetParent(this);
   }

   inline void
   SetRight(Node* node){
     rchild_ = node;
     if(node)
       node->SetParent(this);
   }

   uint256 ComputeHash() const{
     if(IsLeaf())
       return hash_;
     return sha256::Concat(left()->hash(), right()->hash());
   }
  public:
   Node() = delete;
   Node(Node* parent, const uint256& hash):
    parent_(parent),
    lchild_(nullptr),
    rchild_(nullptr),
    hash_(hash){
   }
   Node(const uint256& hash):
    Node(nullptr, hash){
   }
   Node(Node* parent, Node* left, Node* right):
    Node(parent, sha256::Concat(left->hash(), right->hash())){
     SetLeft(left);
     SetRight(right);
   }
   Node(Node* left, Node* right):
    Node(nullptr, left, right){
   }
   Node(const Node& rhs) = delete;
   ~Node(){
     delete lchild_;
     delete rchild_;
   }

   Node* parent() const{
     return parent_;
   }

   bool HasParent() const{
     return parent_ != nullptr;
   }

   Node* left() const{
     return lchild_;
   }

   bool HasLeft() const{
     return lchild_ != nullptr;
   }

   Node* right() const{
     return rchild_;
   }

   bool HasRight() const{
     return rchild_ != nullptr;
   }

   bool IsLeaf() const{
     return !HasLeft() && !HasRight();
   }

   uint256 hash() const{
     return hash_;
   }

   bool CanVerifyHash() const{
     return HasLeft() && HasRight();
   }

   bool VerifyHash() const{
     if(IsLeaf())
       return true;
     if(!HasRight())
       return hash() == left()->hash();
     return hash() == ComputeHash();
   }

   int GetLeaves() const{
     return IsLeaf() ? 1 : left()->GetLeaves() + right()->GetLeaves();
   }

   Node& operator=(const Node& rhs) = delete;

   friend std::ostream& operator<<(std::ostream& stream, const Node& val){
     return stream << val.hash();
   }

   friend bool operator==(const Node& lhs, const Node& rhs){
     return lhs.hash() == rhs.hash();
   }

   friend bool operator!=(const Node& lhs, const Node& rhs){
     return lhs.hash() != rhs.hash();
   }

   friend bool operator<(const Node& lhs, const Node& rhs){
     return lhs.hash() < rhs.hash();
   }

   friend bool operator>(const Node& lhs, const Node& rhs){
     return lhs.hash() > rhs.hash();
   }
 };

 class Tree{
  private:
   Node* root_;

   const Node* Search(const Node* root, const uint256& hash) const;
   Node* BuildTree(std::vector<Node*>& leaves);
   Node* BuildTree(std::vector<uint256>& leaves);
  public:
   Tree(std::vector<uint256>& leaves):
    root_(BuildTree(leaves)){
   }
   virtual ~Tree(){
     if(root_)
       delete root_;
   }

   const Node* root() const{
     return root_;
   }

   bool empty() const{
     return root() != nullptr;
   }

   uint256 GetRootHash() const{
     return root()->hash();
   }

   const Node* GetNode(const uint256& hash) const;
   bool GetNodes(std::vector<uint256>& hashes) const;
   bool GetLeaves(std::vector<uint256>& hashes) const;
   void VisitLeaves(NodeVisitor* vis) const;
   void VisitNodes(NodeVisitor* vis) const;
 };
}

#endif //TOKEN_MERKLE_H
