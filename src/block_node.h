#ifndef TOKEN_BLOCK_NODE_H
#define TOKEN_BLOCK_NODE_H

#include "block.h"

namespace Token{
    class BlockNode : public Object{
    private:
        BlockNode* previous_;
        BlockNode* next_;
        BlockHeader block_;

        BlockNode(const BlockHeader& block):
                previous_(nullptr),
                next_(nullptr),
                block_(block){}
        BlockNode(Block* block): BlockNode(block->GetHeader()){}
    public:
        ~BlockNode(){
            //TODO: implement
        }

        void SetPrevious(BlockNode* node){
            previous_ = node;
        }

        BlockNode* GetPrevious() const{
            return previous_;
        }

        bool HasPrevious() const{
            return previous_ != nullptr;
        }

        void SetNext(BlockNode* node){
            next_ = node;
        }

        BlockNode* GetNext() const{
            return next_;
        }

        bool HasNext() const{
            return next_ != nullptr;
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

        std::string ToString() const;

        static BlockNode* NewInstance(const BlockHeader& block);

        static BlockNode* NewInstance(Block* block){
            return NewInstance(block->GetHeader());
        }
    };
}

#endif //TOKEN_BLOCK_NODE_H