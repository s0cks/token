#ifndef TOKEN_BLOCK_CHAIN_H
#define TOKEN_BLOCK_CHAIN_H

#include <leveldb/db.h>
#include <map>

#include "common.h"
#include "block.h"
#include "unclaimed_transaction.h"

namespace Token{
    class BlockChainVisitor;
    class BlockChain : public IndexManagedPool{
        //TODO:
        // - improve load logic by loading all blocks into sorted vector?
    private:
        friend class BlockMiner;

        class BlockNode{
        private:
            BlockNode* parent_;
            Array<BlockNode*> children_;
            BlockHeader header_;
        public:
            BlockNode(Block* block):
                parent_(nullptr),
                children_(10),
                header_(block){}
            ~BlockNode(){
                if(!IsLeaf()){
                    for(auto& it : children_) delete it;
                }
            }

            BlockNode* GetParent() const{
                return parent_;
            }

            BlockNode* GetChild(uint64_t idx) const{
                if(idx > GetNumberOfChildren()) return nullptr;
                return children_[idx];
            }

            uint64_t GetNumberOfChildren() const{
                return children_.Length();
            }

            uint64_t GetTimestamp() const{
                return header_.GetTimestamp();
            }

            uint64_t GetHeight() const{
                return header_.GetHeight();
            }

            uint256_t GetPreviousHash() const{
                return header_.GetPreviousHash();
            }

            uint256_t GetHash() const{
                return header_.GetHash();
            }

            bool IsLeaf() const{
                return GetNumberOfChildren() == 0;
            }

            void AddChild(BlockNode* child){
                children_.Add(child);
                child->SetParent(this);
            }

            void SetParent(BlockNode* node){
                parent_ = node;
            }
        };

        BlockNode* genesis_;
        BlockNode* head_;
        std::map<uint256_t, BlockNode*> nodes_; //TODO: possible memory leak?

        static BlockChain* GetInstance();
        uint256_t GetHeadFromIndex();
        bool HasHeadInIndex();
        bool SetHeadInIndex(const uint256_t& hash);
        bool LoadBlock(const std::string& filename, Block* block);
        bool SaveBlock(const std::string& filename, Block* block);
        bool Append(Block* block);
        BlockNode* GetHeadNode();
        BlockNode* GetGenesisNode();
        BlockNode* GetNode(const uint256_t& hash);

        BlockChain();
    public:
        ~BlockChain(){}

        static uint64_t GetHeight();
        static uint256_t GetHead();
        static uint256_t GetGenesis();
        static bool Initialize();
        static bool ContainsBlock(const uint256_t& hash);
        static bool ContainsTransaction(const uint256_t& hash);
        static bool Accept(BlockChainVisitor* vis);
        static Block* GetBlock(const uint256_t& hash);
        static Transaction* GetTransaction(const uint256_t& hash);
        static MerkleTree* GetMerkleTree();

        //TODO remove
        static bool AppendBlock(Block* block){
            return GetInstance()->Append(block);
        }
    };

    class BlockChainVisitor{
    public:
        virtual ~BlockChainVisitor() = default;

        virtual bool VisitStart(){ return true; }
        virtual bool VisitBlock(Block* block) = 0;
        virtual bool VisitEnd(){ return true; };
    };
}

#endif //TOKEN_BLOCK_CHAIN_H