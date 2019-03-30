#include "blockchain.h"
#include <fstream>
#include <sstream>
#include "block_validator.h"

namespace Token{
    BlockChain*
    BlockChain::CreateInstance(Token::User *user, const std::string &root){
        Block* genesis = new Block();
        Transaction* tx = genesis->CreateTransaction();
        tx->AddOutput(user->GetUserId(), "TestToken");
        BlockChain* chain = new BlockChain(user, root);
        chain->AppendGenesis(genesis);
        return chain;
    }

    BlockChain*
    BlockChain::CreateInstance(User* user){
        Block* genesis = new Block();
        Transaction* tx = genesis->CreateTransaction();
        for(int i = 0; i <= 128; i++){
            std::stringstream stream;
            stream << "TestToken" << i;
            tx->AddOutput(user->GetUserId(), stream.str());
        }
        BlockChain* chain = new BlockChain(user);
        chain->AppendGenesis(genesis);
        return chain;
    }

    bool
    BlockChain::AppendGenesis(Token::Block* genesis){
        Transaction* cb = genesis->GetCoinbaseTransaction();
        BlockChainNode* node = new BlockChainNode(nullptr, genesis);
        for(int i = 0; i < cb->GetNumberOfOutputs(); i++){
            node->GetUnclainedTransactionPool()->Insert(UnclaimedTransaction(cb->GetHash(), i), cb->GetOutputAt(i));
        }
        heads_->Add(node);
        nodes_.insert(std::make_pair(genesis->GetHash(), node));
        height_ = 1;
        head_ = node;
        txpool_ = new TransactionPool();
        return true;
    }

    static inline bool
    FileExists(const std::string& name){
        std::ifstream f(name.c_str());
        return f.good();
    }

    BlockChain*
    BlockChain::LoadInstance(User* user, const std::string& root){
        BlockChain* chain = new BlockChain(user, root);
        Block* genesis = nullptr;
        std::string genesis_filename = root + "/blk0.dat";
        if(FileExists(genesis_filename)){
            genesis = Block::Load(genesis_filename);
        } else{
            return BlockChain::CreateInstance(user, root);
        }
        chain->AppendGenesis(genesis);
        for(int idx = 1; idx < 1000; idx++){
            std::stringstream blk_filename;
            blk_filename << root;
            blk_filename << "/blk" << idx << ".dat";
            if(FileExists(blk_filename.str())){
                Block* block = Block::Load(blk_filename.str());
                chain->Append(block);

            } else{
                return chain;
            }
        }
        return chain;
    }

    static inline std::string
    GetBlockFilename(const std::string& root, int idx) {
        std::stringstream ss;
        ss << root;
        ss << "/blk" << idx << ".dat";
        return ss.str();
    }

    void
    BlockChain::Write(const std::string &root){
        for(int idx = 0; idx <= GetHeight(); idx++){
            Block* blk = GetBlockAt(idx);
            blk->Write(GetBlockFilename(root, idx));
        }
    }

    bool BlockChain::Append(Token::Block* block){
        if(nodes_.find(block->GetHash()) != nodes_.end()) return false;
        if(block->GetHeight() == 0) return AppendGenesis(block);
        if(nodes_.find(block->GetPreviousHash()) == nodes_.end()) return false;
        BlockChainNode* parent = GetBlockParent(block);
        if(parent == nullptr) return false;

        
        UnclaimedTransactionPool utxo_pool = *parent->GetUnclainedTransactionPool();
        /*
        BlockValidator validator(utxo_pool);
        block->Accept(&validator);
        std::vector<Transaction*> valid = validator.GetValidTransactions();
        std::vector<Transaction*> invalid = validator.GetInvalidTransactions();
        if(valid.size() != block->GetNumberOfTransactions()){
            return false;
        }
        utxo_pool = validator.GetUnclaimedTransactionPool();
        if(utxo_pool.IsEmpty()) {
            return false;
        }
        */

        Transaction* cb = block->GetCoinbaseTransaction();
        utxo_pool.Insert(UnclaimedTransaction(cb->GetHash(), 0), cb->GetOutputAt(0));
        BlockChainNode* current = new BlockChainNode(parent, block, utxo_pool);
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
        if(!root_.empty()){
            block->Write(GetBlockFilename(root_, block->GetHeight()));
        }
        return true;
    }
}