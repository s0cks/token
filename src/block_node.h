#ifndef TOKEN_BLOCK_NODE_H
#define TOKEN_BLOCK_NODE_H

#include "block.h"

namespace Token{
    class BlockNode{
    private:
        BlockNode* previous_;
        BlockNode* next_;
        BlockHeader block_;

        void SetPrevious(BlockNode* node){
            previous_ = node;
        }

        void SetNext(BlockNode* node){
            next_ = node;
        }

        friend class BlockChain;
    public:
        BlockNode(const BlockHeader& block):
            previous_(nullptr),
            next_(nullptr),
            block_(block){}
        BlockNode(Block* block): BlockNode(block->GetHeader()){}
        ~BlockNode(){
            //TODO: implement
        }

        BlockNode* GetPrevious() const{
            return previous_;
        }

        BlockNode* GetNext() const{
            return next_;
        }

        BlockHeader GetBlock() const{
            return block_;
        }

        uint32_t GetTimestamp() const{
            return block_.GetTimestamp();
        }

        uint32_t GetHeight() const{
            return block_.GetHeight();
        }

        uint256_t GetPreviousHash() const{
            return block_.GetPreviousHash();
        }

        uint256_t GetMerkleRoot() const{
            return block_.GetMerkleRoot();
        }

        uint256_t GetHash() const{
            return block_.GetHash();
        }
    };
}

#endif //TOKEN_BLOCK_NODE_H