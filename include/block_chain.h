#ifndef TOKEN_BLOCK_CHAIN_H
#define TOKEN_BLOCK_CHAIN_H

#include <leveldb/db.h>
#include <map>
#include "blockchain.pb.h"
#include "common.h"
#include "object.h"
#include "pool.h"

namespace Token{
    class BlockChainVisitor;
    class BlockChain : public IndexManagedPool<Block>{
    private:
        friend class BlockChainServer;
        friend class BlockMiner;

        class BlockNode{
        private:
            BlockNode* parent_;
            std::vector<BlockNode*> children_;
            BlockHeader header_;
        public:
            BlockNode(const Block& block):
                parent_(nullptr),
                children_(),
                header_(block){}
            ~BlockNode(){
                if(!IsLeaf()){
                    for(auto& it : children_) delete it;
                }
            }

            BlockHeader GetHeader() const{
                return header_;
            }

            BlockNode* GetParent() const{
                return parent_;
            }

            BlockNode* GetChild(uint64_t idx) const{
                if(idx > GetNumberOfChildren()) return nullptr;
                return children_[idx];
            }

            uint64_t GetNumberOfChildren() const{
                return children_.size();
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
                children_.push_back(child);
                child->SetParent(this);
            }

            void SetParent(BlockNode* node){
                parent_ = node;
            }
        };

        BlockNode* genesis_;
        BlockNode* head_;
        std::map<uint256_t, BlockNode*> nodes_; //TODO: possible memory leak?
        pthread_rwlock_t rwlock_;

        static BlockChain* GetInstance();
        uint256_t GetHeadFromIndex();
        bool HasHeadInIndex();
        bool SetHeadInIndex(const uint256_t& hash);
        bool Append(Block* block);

        std::string CreateObjectLocation(const uint256_t& hash, Block* block) const{
            std::stringstream filename;
            filename << GetRoot() << "/blk" << block->GetHeight() << ".dat";
            return filename.str();
        }

        static BlockNode* GetHeadNode();
        static BlockNode* GetGenesisNode();
        static BlockNode* GetNode(const uint256_t& hash);

        BlockChain();
    public:
        ~BlockChain(){}

        static uint64_t GetHeight(){
            return GetHead().GetHeight();
        }

        static BlockHeader GetHead(){
            return GetHeadNode()->GetHeader();
        }

        static BlockHeader GetGenesis(){
            return GetGenesisNode()->GetHeader();
        }

        static BlockHeader GetBlock(const uint256_t& hash){
            return GetNode(hash)->GetHeader();
        }

        static bool ContainsBlock(const uint256_t& hash){
            return GetInstance()->GetNode(hash) != nullptr;
        }

        static bool Initialize();
        static bool Accept(BlockChainVisitor* vis);
        static bool GetBlockData(const uint256_t& hash, Block* result);
        static bool GetTransaction(const uint256_t& hash, Transaction* result);
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
        virtual bool Visit(const Block& block) = 0;
        virtual bool VisitEnd(){ return true; };
    };
}

#endif //TOKEN_BLOCK_CHAIN_H