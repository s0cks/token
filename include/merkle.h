#ifndef TOKEN_MERKLE_H
#define TOKEN_MERKLE_H

#include <vector>
#include "allocator.h"
#include "object.h"
#include "uint256_t.h"

namespace Token{
    class BlockVisitor;
    class Block;

    //TODO: offload to heap
    class MerkleNode{
    protected:
        MerkleNode* parent_;
        MerkleNode* left_;
        MerkleNode* right_;
        uint256_t hash_;

        void SetParent(MerkleNode* node){
            parent_ = node;
        }

        uint256_t ComputeHash() const;
    public:
        MerkleNode(const uint256_t& hash):
                parent_(nullptr),
                left_(nullptr),
                right_(nullptr),
                hash_(hash){}
        MerkleNode(MerkleNode* node):
                parent_(nullptr),
                left_(nullptr),
                right_(nullptr),
                hash_(){
            SetParent(node->GetParent());
            SetLeft(node->GetLeft());
            SetRight(node->GetRight());
            hash_ = ComputeHash();
        }
        MerkleNode(MerkleNode* left, MerkleNode* right):
                parent_(nullptr),
                left_(nullptr),
                right_(nullptr),
                hash_(){
            SetLeft(left);
            SetRight(right);
            hash_ = ComputeHash();
        }
        ~MerkleNode() = default;

        MerkleNode* GetParent() const{
            return parent_;
        }

        bool HasParent() const{
            return parent_ != nullptr;
        }

        void SetLeft(MerkleNode* node){
            left_ = node;
            if(left_)left_->SetParent(this);
        }

        MerkleNode* GetLeft() const{
            return left_;
        }

        bool Finalize(){
            return true;
        }

        bool HasLeft() const{
            return left_ != nullptr;
        }

        void SetRight(MerkleNode* node){
            right_ = node;
            if(right_) right_->SetParent(this);
        }

        MerkleNode* GetRight() const{
            return right_;
        }

        bool HasRight() const{
            return right_ != nullptr;
        }

        bool IsLeaf() const{
            return !HasLeft() && !HasRight();
        }

        uint256_t GetHash() const{
            return hash_;
        }

        bool Equals(MerkleNode* node) const{
            return hash_ == node->hash_;
        }

        bool CanVerifyHash() const{
            return (HasLeft() && HasRight()) || HasLeft();
        }

        bool VerifyHash() const;

        uint64_t GetLeaves() const{
            if(IsLeaf()) return 1;
            return GetLeft()->GetLeaves() + GetRight()->GetLeaves();
        }

        std::string ToString() const;
    };

    //TODO: refactor MerkleProofHash?
    class MerkleProofHash{
    public:
        enum class Direction{
            kRoot, kLeft, kRight,
        };
    private:
        Direction direction_;
        uint256_t hash_;
    public:
        MerkleProofHash(Direction direction, const uint256_t& hash):
                direction_(direction),
                hash_(hash){}
        MerkleProofHash(Direction direction, const MerkleNode& node):
                direction_(direction),
                hash_(node.GetHash()){}
        MerkleProofHash(const MerkleProofHash& other):
            direction_(other.direction_),
            hash_(other.hash_){}
        ~MerkleProofHash(){}

        Direction GetDirection() const{
            return direction_;
        }

        bool IsRoot() const{
            return GetDirection() == Direction::kRoot;
        }

        bool IsLeft() const{
            return GetDirection() == Direction::kLeft;
        }

        bool IsRight() const{
            return GetDirection() == Direction::kRight;
        }

        uint256_t GetHash() const{
            return hash_;
        }

        MerkleProofHash& operator=(const MerkleProofHash& other){
            direction_ = other.direction_;
            hash_ = other.hash_;
            return (*this);
        }
    };

    class MerkleTree{
    private:
        MerkleNode* root_;
        std::vector<MerkleNode*> leaves_;
        std::map<uint256_t, MerkleNode*> nodes_;

        MerkleNode* BuildMerkleTree(size_t height, std::vector<MerkleNode*>& nodes);
        MerkleNode* BuildMerkleTree(std::vector<uint256_t>& leaves);
    public:
        MerkleTree(std::vector<uint256_t>& leaves):
                root_(nullptr),
                nodes_(),
                leaves_(){
            root_ = BuildMerkleTree(leaves);
        }
        ~MerkleTree() = default;

        void Clear(){
            if(IsEmpty()) return;
            root_ = nullptr;
            nodes_.clear();
            leaves_.clear();
        }

        bool HasMerkleRoot() const{
            return GetMerkleRoot() != nullptr;
        }

        bool IsEmpty() const{
            return !HasMerkleRoot() && leaves_.size() > 0;
        }

        MerkleNode* GetMerkleRoot() const {
            return root_;
        }

        bool GetLeaves(std::vector<uint256_t>& leaves) const{
            if(leaves_.empty()) return false;
            for(auto& it : leaves_) leaves.push_back(it->GetHash());
            return leaves.size() > 0;
        }

        uint256_t GetMerkleRootHash() const{
            if(!HasMerkleRoot()) return uint256_t();
            return GetMerkleRoot()->GetHash();
        }

        std::string ToString() const;
        MerkleNode* GetNode(const uint256_t& hash) const;
        MerkleNode* GetLeafNode(const uint256_t& hash) const;
        bool Finalize();
        bool Append(const MerkleTree& tree);

        bool BuildAuditProof(const uint256_t& hash, std::vector<MerkleProofHash>& trail);
        bool VerifyAuditProof(const uint256_t& root, const uint256_t& leaf, std::vector<MerkleProofHash>& trail);

        bool BuildConsistencyProof(uint64_t num_nodes, std::vector<MerkleProofHash>& trail);
        bool BuildConsistencyAuditProof(const uint256_t& hash, std::vector<MerkleProofHash>& trail);
        bool VerifyConsistencyProof(const uint256_t& root, std::vector<MerkleProofHash>& trail);
    };

    //TODO: refactor MerkleTreeBuilder class?
    class MerkleTreeBuilder{
    protected:
        std::vector<uint256_t> leaves_;
        MerkleTree* tree_;

        bool CreateTree(){
            if(leaves_.empty()) return false;
            if(leaves_.size() == 1) leaves_.push_back(leaves_.front());
            tree_ = new MerkleTree(leaves_);
            return HasTree();
        }

        bool AddLeaf(const uint256_t& hash){
            leaves_.push_back(hash);
            return true; //maybe check if value is in vector?
        }

        MerkleTreeBuilder():
            leaves_(),
            tree_(nullptr){}
    public:
        virtual ~MerkleTreeBuilder() = default;

        MerkleTree* GetTree(){
            return tree_;
        }

        bool HasTree(){
            return tree_ && tree_->HasMerkleRoot();
        }

        virtual bool BuildTree() = 0;
    };
}

#endif //TOKEN_MERKLE_H