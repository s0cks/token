#include "blockchain.h"

namespace Token{
    Block* BlockChain::CreateBlock(Token::User* user){
        Block* parent = GetHead();
        Block* current = new Block(parent);
        return current;
    }

    bool BlockChain::AddBlock(Token::Block* block){
        if(nodes_.find(block->GetPreviousHash()) == nodes_.end()) return false;
        //TODO: Transaction Verification Logic
        BlockChainNode* parent = nodes_[block->GetPreviousHash()];
        UnclaimedTransactionPool* utxpool = parent->GetNewUnclaimedTransactionPool();
        BlockChainNode* current = new BlockChainNode(parent, block, utxpool);
        nodes_.insert(std::make_pair(block->GetHash(), current));
        if(current->GetHeight() > GetHeight()) {
            height_ = current->GetHeight();
            head_ = current;
        }
        if(GetHeight() - (*heads_)[0]->GetHeight() > 0xA){
            Array<BlockChainNode*>* newHeads = new Array<BlockChainNode*>(0xA);
            for(size_t i = 0; i < heads_->Length(); i++){
                BlockChainNode* oldHead = (*heads_)[i];
                Array<BlockChainNode*> oldHeadChildren = oldHead->GetChildren();
                for(size_t j = 0; j < oldHeadChildren.Length(); j++){
                    BlockChainNode* oldHeadChild = oldHeadChildren[j];
                    newHeads->Add(oldHeadChild);
                }
                nodes_.erase(oldHead->GetBlock()->GetHash());
            }
            heads_ = newHeads;
        }
        return true;
    }
}