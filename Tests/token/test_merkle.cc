#include <gtest/gtest.h>
#include <glog/logging.h>

#include "helpers.h"
#include "token/merkle.h"

namespace token{
 using namespace token::merkle;

 static inline ::testing::AssertionResult
 HasNode(const Tree* tree, const uint256& hash){
   auto node = tree->GetNode(hash);
   if(node == nullptr)
     return ::testing::AssertionFailure() << "Cannot find node '" << hash << "' in Merkle Tree.";
   return ::testing::AssertionSuccess();
 }

 static inline ::testing::AssertionResult
 HasNode(const Tree* tree, const std::string& hash){
   return HasNode(tree, sha256::FromHex(hash));
 }

 static inline ::testing::AssertionResult
 IsRoot(const Tree* tree, const uint256& hash){
   auto root = tree->GetRootHash();
   if(root != hash)
     return ::testing::AssertionFailure() << "Merkle Tree root (" << root << ") does not equal " << hash;
   return ::testing::AssertionSuccess();
 }

 static inline ::testing::AssertionResult
 IsRoot(const Tree* tree, const std::string& hash){
   return IsRoot(tree, sha256::FromHex(hash));
 }

 class MerkleTreeTest : public ::testing::Test{
  public:
   static constexpr const char* kNode1 = "4334414543333542313831393935423042384439334630313643453731333044";
   static constexpr const char* kNode2 = "3044393341393434303038423731423237364636344243344143434641423633";
   static constexpr const char* kRoot = "8054033054C49C29C2766F7ABCA082DFCA0A28E0093DE33D8F9D9F11726ADF4B";
  protected:
   Tree* tree_;

   inline const Tree*
   tree() const{
     return tree_;
   }

   void SetUp() override{
     std::vector<uint256> leaves = {
       sha256::FromHex(kNode1),
       sha256::FromHex(kNode2),
     };
     tree_ = new Tree(leaves);
   }
  public:
   MerkleTreeTest() = default;
   ~MerkleTreeTest() = default;
 };

 TEST_F(MerkleTreeTest, TestGetNode){
   ASSERT_TRUE(HasNode(tree(), kNode1));
   ASSERT_TRUE(HasNode(tree(), kNode2));
   ASSERT_TRUE(HasNode(tree(), kRoot));
   ASSERT_FALSE(HasNode(tree(), sha256::Nonce(1024)));
 }

 TEST_F(MerkleTreeTest, TestRoot){
   ASSERT_TRUE(IsRoot(tree(), kRoot));
 }
}