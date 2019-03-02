#ifndef TOKEN_BLOCKCHAIN_H
#define TOKEN_BLOCKCHAIN_H

#include "common.h"
#include "array.h"
#include "block.h"
#include "tx_pool.h"
#include "utx_pool.h"
#include "user.h"

namespace Token{
    class BlockChainNode{
    private:
        Block* block_;
        BlockChainNode* parent_;
        Array<BlockChainNode*> children_;
        UnclaimedTransactionPool* utxpool_;
        unsigned int height_;

        inline void
        AddChild(BlockChainNode* node){
            children_.Add(node);
        }

        friend class BlockChain;
    public:
        BlockChainNode(BlockChainNode* parent, Block* block, UnclaimedTransactionPool* utxpool):
            parent_(parent),
            block_(block),
            height_(0),
            utxpool_(utxpool),
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

        UnclaimedTransactionPool* GetUnclaimedTransactionPool() const{
            return utxpool_;
        }

        UnclaimedTransactionPool* GetNewUnclaimedTransactionPool() const{
            return new UnclaimedTransactionPool(*utxpool_);
        }
    };

    class BlockChain{
    private:
        Array<BlockChainNode*>* heads_;
        BlockChainNode* head_;
        std::map<std::string, BlockChainNode*> nodes_;
        unsigned int height_;
        TransactionPool* txpool_;
    public:
        BlockChain(Block* genesis):
            heads_(new Array<BlockChainNode*>(0xA)),
            nodes_(),
            height_(0){

            UnclaimedTransactionPool* utxpool = new UnclaimedTransactionPool(this);
            Transaction* cbtx = genesis->CreateTransaction();
            BlockChainNode* node = new BlockChainNode(nullptr, genesis, utxpool);
            heads_->Add(node);
            nodes_.insert(std::make_pair(genesis->GetHash(), node));
            height_ = 1;
            head_ = node;
            txpool_ = new TransactionPool();
        }

        Block* CreateBlock(User* user);

        Block* GetBlockFromHash(const std::string& hash){
            return nodes_[hash]->GetBlock();
        }

        Block* GetHead() const{
            return head_->GetBlock();
        }

        UnclaimedTransactionPool* GetHeadUnclaimedTransactionPool() const{
            return head_->GetUnclaimedTransactionPool();
        }

        unsigned int GetHeight() const{
            return height_;
        }

        TransactionPool* GetTransactionPool() const{
            return txpool_;
        }

        void AddTransaction(Transaction* tx){
            GetTransactionPool()->AddTransaction(tx);
        }

        bool AddBlock(Block* block);
    };
}

#endif //TOKEN_BLOCKCHAIN_H