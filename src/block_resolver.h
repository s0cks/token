#ifndef TOKEN_BLOCK_RESOLVER_H
#define TOKEN_BLOCK_RESOLVER_H

#include "blockchain.h"

namespace Token{
    class BlockResolver : public BlockChainVisitor{
    private:
        std::string target_;
        Block* result_;

        inline void
        SetResult(Block* block){
            result_ = block;
        }
    public:
        BlockResolver(std::string target):
            target_(target),
            result_(nullptr){}
        ~BlockResolver(){}

        std::string GetTarget() const{
            return target_;
        }

        void Visit(Block* block){
            if(HasResult()) return;
            if(GetTarget() == block->GetHash()){
                SetResult(block);
            }
        }

        Block* GetResult() const{
            return result_;
        }

        bool HasResult() const{
            return result_ != nullptr;
        }
    };
}

#endif //TOKEN_BLOCK_RESOLVER_H