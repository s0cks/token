#ifndef TOKEN_BLOCKCHAIN_H
#define TOKEN_BLOCKCHAIN_H

#include "common.h"
#include "array.h"
#include "block.h"
#include "tx_pool.h"
#include "user.h"

namespace Token{
    class BlockChainNode{
    private:
        Block* block_;
        BlockChainNode* parent_;
        Array<BlockChainNode*> children_;
        unsigned int height_;

        inline void
        AddChild(BlockChainNode* node){
            children_.Add(node);
        }

        friend class BlockChain;
    public:
        BlockChainNode(BlockChainNode* parent, Block* block):
            parent_(parent),
            block_(block),
            height_(1),
            children_(0xA){
            if(parent_ != nullptr){
                height_ = parent->GetHeight() + 1;
                parent->AddChild(this);
            }
        }

        Block* GetBlock() const{
            return block_;
        }

        Array<BlockChainNode*> GetChildren() const{
            return children_;
        }

        unsigned int GetHeight() const{
            return height_;
        }
    };

    class BlockChain{
    private:
        Array<BlockChainNode*>* heads_;
        BlockChainNode* head_;
        std::map<std::string, BlockChainNode*> nodes_;
        unsigned int height_;
        TransactionPool* txpool_;
        User* owner_;
        std::string root_;

        bool AppendGenesis(Block* block);

        BlockChain(User* owner, std::string root=""):
            root_(),
            owner_(owner),
            heads_(new Array<BlockChainNode*>(0xA)),
            nodes_(),
            height_(0){}
    public:
        static BlockChain* CreateInstance(User* user);
        static BlockChain* CreateInstance(User* user, const std::string& root);
        static BlockChain* LoadInstance(User* user, const std::string& root);

        User* GetOwner() const{
            return owner_;
        }

        Block* CreateBlock() const{
            return new Block(GetHead());
        }

        Block* GetBlockFromHash(const std::string& hash) const{
            return nodes_.find(hash)->second->GetBlock();
        }

        Block* GetHead() const{
            return head_->GetBlock();
        }

        Block* GetBlockAt(int height) const{
            if(height > GetHeight()) return nullptr;
            Block* head = GetHead();
            while(head && head->GetHeight() > height) head = GetBlockFromHash(head->GetPreviousHash());
            return head;
        }

        unsigned int GetHeight() const{
            return height_;
        }

        TransactionPool* GetTransactionPool() const{
            return txpool_;
        }

        bool Append(Block* block);
        void Write(const std::string& root);
    };
}

#endif //TOKEN_BLOCKCHAIN_H