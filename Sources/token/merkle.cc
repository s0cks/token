#include "token/common.h"
#include "token/merkle.h"

namespace token::merkle{
 Node* Tree::BuildTree(std::vector<Node*>& nodes){
   if(nodes.size() == 1)
     return nodes.front();
   std::vector<Node*> parents;
   for(auto idx = 0; idx < nodes.size(); idx += 2)
     parents.push_back(new Node(nodes[idx], nodes[idx + 1]));
   return BuildTree(parents);
 }

 Node* Tree::BuildTree(std::vector<uint256>& leaves){
   std::vector<Node*> nodes;
   for(auto& it : leaves)
     nodes.push_back(new Node(it));
   if((nodes.size() % 2) == 1)
     nodes.push_back(nodes.back());//TODO: should we make a copy of this?
   return BuildTree(nodes);
 }

 const Node* Tree::Search(const Node* root, const uint256& hash) const{
   if(root == nullptr || root->hash() == hash)
     return root;
   auto node = Search(root->left(), hash);
   if(node != nullptr)
     return node;
   node = Search(root->right(), hash);
   return node;
 }

 const Node* Tree::GetNode(const uint256& hash) const{
   return Search(root(), hash);
 }

 bool Tree::GetNodes(std::vector<uint256>& hashes) const{
   NOT_IMPLEMENTED(FATAL);//TODO: implement
   return false;
 }

 bool Tree::GetLeaves(std::vector<uint256>& hashes) const{
   NOT_IMPLEMENTED(FATAL);//TODO: implement
   return false;
 }

 void Tree::VisitNodes(NodeVisitor* vis) const{
   NOT_IMPLEMENTED(FATAL);//TODO: implement
 }

 void Tree::VisitLeaves(NodeVisitor* vis) const{
   NOT_IMPLEMENTED(FATAL);//TODO: implement
 }
}