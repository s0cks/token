#include "blockchain.h"
#include <fstream>
#include <sstream>
#include "block_validator.h"

namespace Token{
    bool
    BlockChain::AppendGenesis(Token::Block* genesis){
        Transaction* cb = genesis->GetCoinbaseTransaction();
        BlockChainNode* node = new BlockChainNode(nullptr, genesis, new UnclaimedTransactionPool());
        for(int i = 0; i < cb->GetNumberOfOutputs(); i++){
            node->GetUnclainedTransactionPool()->Insert(cb->GetHash(), i, cb->GetOutputAt(i));
        }
        heads_->Add(node);
        nodes_.insert({ genesis->GetHash(), node });
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
    BlockChain::GetInstance(){
        static BlockChain instance;
        return &instance;
    }

    bool
    BlockChain::Load(const std::string& root, const std::string& addr, int port){
        Load(root);
        std::cout << "Fetching head from " << addr << ":" << port << std::endl;
        return true;
    }

    bool
    BlockChain::Load(const std::string& root){
        Block* genesis = nullptr;

        std::string gen_filename = root + "/blk0.dat";
        if(FileExists(gen_filename)){
            genesis = Block::Load(gen_filename);
            AppendGenesis(genesis);
        } else{
            genesis = new Block(true);
            Transaction* tx = genesis->CreateTransaction();
            for(int idx = 0; idx < 128; idx++){
                std::stringstream token_name;
                token_name << "Token" << idx;
                tx->AddOutput("TestUser", token_name.str());
            }
            AppendGenesis(genesis);
            return false;
        }
        for(int idx = 1; idx < 1000; idx++){
            std::stringstream blk_filename;
            blk_filename << root << "/blk" << idx << ".dat";
            if(FileExists(blk_filename.str())){
                Append(Block::Load(blk_filename.str()));
            } else{
                break;
            }
        }
        return true;
    }

    static inline std::string
    GetBlockFilename(const std::string& root, int idx) {
        std::stringstream ss;
        ss << root;
        ss << "/blk" << idx << ".dat";
        return ss.str();
    }

    bool
    BlockChain::Save(const std::string& root){
        for(int idx = 0; idx <= GetHeight(); idx++){
            Block* blk = GetBlockAt(idx);
            blk->Write(GetBlockFilename(root, idx));
        }
        return true;
    }

    bool BlockChain::Append(Token::Block* block){
        std::cout << "Finding duplicates" << std::endl;
        if(nodes_.find(block->GetHash()) != nodes_.end()){
            std::cout << "Duplicate block found" << std::endl;
            return false;
        }
        std::cout << "Checking height" << std::endl;
        if(block->GetHeight() == 0) return AppendGenesis(block);
        std::cout << "Finding parent" << std::endl;
        if(nodes_.find(block->GetPreviousHash()) == nodes_.end()){
            std::cout << "Cannot find parent" << std::endl;
            return false;
        }
        BlockChainNode* parent = GetBlockParent(block);
        std::cout << "Getting parent's UTXOs" << std::endl;
        UnclaimedTransactionPool* utxo_pool = parent->GetUnclainedTransactionPool();

        BlockValidator validator(utxo_pool);
        block->Accept(&validator);
        std::vector<Transaction*> valid = validator.GetValidTransactions();
        std::vector<Transaction*> invalid = validator.GetInvalidTransactions();
        if(valid.size() != block->GetNumberOfTransactions()){
            std::cerr << "Not all transactions valid" << std::endl;
            return false;
        }
        utxo_pool = validator.GetUnclaimedTransactionPool();
        if(utxo_pool->IsEmpty()) {
            std::cout << "No Unclaimed transactions" << std::endl;
            return false;
        }

        std::cout << "Processing transactions" << std::endl;
        Transaction* cb = block->GetCoinbaseTransaction();
        std::cout << "Creating CB UTXO" << std::endl;
        utxo_pool->Insert(cb->GetHash(), 0, cb->GetOutputAt(0));
        std::cout << "Creating BlockNode" << std::endl;
        BlockChainNode* current = new BlockChainNode(parent, block, new UnclaimedTransactionPool(utxo_pool));
        std::cout << "Inserting BlockNode" << std::endl;
        nodes_.insert(std::make_pair(block->GetHash(), current));
        std::cout << "Checking Height" << std::endl;
        if(current->GetHeight() > GetHeight()) {
            height_ = current->GetHeight();
            head_ = current;
        }
        std::cout << "Checking heights" << std::endl;
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
        std::cout << "Done" << std::endl;
        return true;
    }
}