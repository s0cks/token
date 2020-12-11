#ifndef TOKEN_MERKLE_H
#define TOKEN_MERKLE_H

#include <vector>
#include <memory>
#include "hash.h"
#include "block.h"

namespace Token{
    class BlockVisitor;
    class MerkleNode{
    protected:
        MerkleNode* parent_;
        MerkleNode* lchild_;
        MerkleNode* rchild_;
        Hash hash_;

        void SetParent(MerkleNode* node){
            parent_ = node;
        }

        Hash ComputeHash() const;
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

        void SetLeft(MerkleNode* node){
            lchild_ = node;
            if(node)
                node->SetParent(this);
        }

        MerkleNode* GetLeft() const{
            return lchild_;
        }

        bool HasLeft() const{
            return lchild_ != nullptr;
        }

        void SetRight(MerkleNode* node){
            rchild_ = node;
            if(node)
                node->SetParent(this);
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

        bool VerifyHash() const;

        int GetLeaves() const{
            if(IsLeaf()) return 1;
            return GetLeft()->GetLeaves() + GetRight()->GetLeaves();
        }

        std::string ToString() const;
    };

    typedef MerkleNode* MerkleNodePtr;

    //TODO: refactor MerkleProofHash?
    class MerkleProofHash{
    public:
        enum Direction{
            kRoot,
            kLeft,
            kRight,
        };
    private:
        Direction direction_;
        Hash hash_;
    public:
        MerkleProofHash(Direction direction, const Hash& hash):
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

        Hash GetHash() const{
            return hash_;
        }

        void operator=(const MerkleProofHash& other){
            direction_ = other.direction_;
            hash_ = other.hash_;
        }

        friend bool operator==(const MerkleProofHash& a, const MerkleProofHash& b){
            return a.hash_ == b.hash_
                && a.direction_ == b.direction_;
        }

        friend bool operator!=(const MerkleProofHash& a, const MerkleProofHash& b){
            return a.hash_ != b.hash_
                && a.direction_ != b.direction_;
        }
    };

    typedef std::vector<MerkleProofHash> MerkleProof;

    static inline void
    PrintProof(const MerkleProof& proof){
        LOG(INFO) << "Proof (" << proof.size() << "):";
        for(auto& it : proof){
            LOG(INFO) << " - " << it.GetHash();
        }
    }

    class MerkleTreeVisitor;
    class MerkleTree{
        friend class MerkleTreeBuilder;
    private:
        MerkleNodePtr root_;
        std::vector<Hash> leaves_;
        std::map<Hash, MerkleNode*> nodes_;

        void SetRoot(MerkleNode* node){
            root_ = node;
        }

        inline MerkleNodePtr
        CreateNode(const Hash& hash){
            MerkleNodePtr node = new MerkleNode(hash);
            nodes_.insert({ hash, node });
            return node;
        }

        inline MerkleNodePtr
        CreateNode(MerkleNodePtr lchild, MerkleNodePtr rchild){
            MerkleNodePtr node = new MerkleNode(lchild, rchild);
            nodes_.insert({ node->GetHash(), node });
            return node;
        }

        MerkleNodePtr BuildTree(std::vector<MerkleNodePtr>& nodes);
        MerkleNodePtr BuildTree(std::vector<Hash>& nodes);
    public:
        MerkleTree(const std::vector<Hash>& leaves):
            root_(nullptr),
            leaves_(leaves),
            nodes_(){
            SetRoot(BuildTree(leaves_));
        }
        ~MerkleTree() = default;

        MerkleNodePtr GetRoot() const{
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

        MerkleNodePtr GetNode(const Hash& hash) const;
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
        virtual bool Visit(const MerkleNodePtr& node) const = 0;
        virtual bool VisitEnd() const{ return true; }
    };

    class MerkleTreeLogger : public MerkleTreeVisitor{
    private:
        google::LogSeverity severity_;
    public:
        MerkleTreeLogger(const google::LogSeverity& severity=google::INFO):
            MerkleTreeVisitor(),
            severity_(severity){}
        ~MerkleTreeLogger() = default;

        bool Visit(const MerkleNodePtr& node) const{
            LOG_AT_LEVEL(severity_) << node->GetHash();
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

#endif //TOKEN_MERKLE_H