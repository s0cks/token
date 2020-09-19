#ifndef TOKEN_BLOCK_NODE_H
#define TOKEN_BLOCK_NODE_H

#include "block.h"

namespace Token{
    class BlockNode{
    private:
        BlockNode* previous_;
        BlockNode* next_;
        BlockHeader value_;
    public:
        BlockNode(const BlockHeader& value):
            previous_(nullptr),
            next_(nullptr),
            value_(value){}
        BlockNode(const Handle<Block>& blk):
            BlockNode(blk->GetHeader()){}
        ~BlockNode() = default;

        bool HasPrevious() const{
            return previous_ != nullptr;
        }

        BlockNode* GetPrevious() const{
            return previous_;
        }

        void SetPrevious(BlockNode* previous){
            previous_ = previous;
        }

        bool HasNext() const{
            return next_ != nullptr;
        }

        BlockNode* GetNext() const{
            return next_;
        }

        void SetNext(BlockNode* node){
            next_ = node;
        }

        BlockHeader GetValue() const{
            return value_;
        }
    };
}

#endif //TOKEN_BLOCK_NODE_H