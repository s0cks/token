#include "blockchain.h"
#include <fstream>
#include <sstream>
#include "block_validator.h"

namespace Token{
    bool BlockChain::AppendGenesis(Token::Block* genesis){
        Transaction* cb = genesis->GetCoinbaseTransaction();
        BlockChainNode* node = new BlockChainNode(nullptr, genesis);
        std::string hash = cb->GetHash();
        for(int i = 0; i < cb->GetNumberOfOutputs(); i++){
            UnclaimedTransaction utxo(cb->GetHash(), i, cb->GetOutputAt(i));
            if(!UnclaimedTransactionPool::GetInstance()->AddUnclaimedTransaction(&utxo)){
                std::cerr << "Cannot append unclaimed transaction" << std::endl;
            }
        }
        heads_->Add(node);
        nodes_.insert({ genesis->GetHash(), node });
        head_ = node;
        GetState()->SetHeight(genesis->GetHeight());
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

    bool BlockChain::Initialize(const std::string &path, Token::Block *genesis){
        BlockChain* instance = BlockChain::GetInstance();
        instance->SetState(new BlockChainState(path));
        if(!UnclaimedTransactionPool::LoadUnclaimedTransactionPool(instance->GetState()->GetUnclaimedTransactionPoolFile())){
            std::cout << "Cannot load unclaimed transaction pool" << std::endl;
            return false;
        }

        if(!instance->AppendGenesis(genesis)){
            std::cout << "Cannot set head" << std::endl;
            return false;
        }
        return instance->Save();
    }

    bool
    BlockChain::Load(const std::string& root){
        if(BlockChainState::CanLoadStateFrom(root)){
            SetState(BlockChainState::LoadState(root));
            if(!UnclaimedTransactionPool::LoadUnclaimedTransactionPool(GetState()->GetUnclaimedTransactionPoolFile())){
                std::cout << "Cannot load unclaimed transaction pool" << std::endl;
                return false;
            }

            int height = GetHeight();
            for(int i = 0; i <= height; i++){
                std::string blk_filename = GetBlockDataFile(i);
                if(!FileExists(blk_filename)){
                    std::cout << "Cannot load block " << i << std::endl;
                    return false;
                }
                std::cout << "Loading block: " << blk_filename << std::endl;
                Block* nblock = Block::Load(blk_filename);
                if(!Append(nblock)){
                    std::cout << "Cannot append block" << std::endl;
                    return false;
                }
            }
            return true;
        }
        Block* genesis = new Block(true);
        Transaction* cbtx = genesis->CreateTransaction();
        for(int i = 0; i < 128; i++){
            std::stringstream stream;
            stream << "Token" << i;
            cbtx->AddOutput(stream.str(), "TestUser");
        }
        return Initialize(root, genesis);
    }

    bool
    BlockChain::Save(){
        GetState()->SaveState();
        for(int idx = 0; idx <= GetHeight(); idx++){
            GetBlockAt(idx)->Write(GetBlockDataFile(idx));
        }
        return true;
    }

    bool BlockChain::Append(Token::Block* block){
        if(nodes_.find(block->GetHash()) != nodes_.end()){
            std::cout << "Duplicate block found" << std::endl;
            return false;
        }
        if(block->GetHeight() == 0) return AppendGenesis(block);
        auto parent_node = nodes_.find(block->GetPreviousHash());
        if(parent_node == nodes_.end()){
            std::cout << "Cannot find parent" << std::endl;
            return false;
        }
        BlockChainNode* parent = GetBlockParent(block);

        BlockValidator validator;
        block->Accept(&validator);
        std::vector<Transaction*> valid = validator.GetValidTransactions();
        std::vector<Transaction*> invalid = validator.GetInvalidTransactions();
        if(valid.size() != block->GetNumberOfTransactions()){
            std::cerr << "Not all transactions valid" << std::endl;
            return false;
        }

        BlockChainNode* current = new BlockChainNode(parent, block);
        nodes_.insert(std::make_pair(block->GetHash(), current));
        if(current->GetHeight() > GetHeight()) {
            GetState()->SetHeight(current->GetHeight());
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

    Block* BlockChain::CreateBlock(){
        Block* parent = GetHead();
        return new Block(parent);
    }
}