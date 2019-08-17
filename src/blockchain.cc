#include <sstream>
#include <glog/logging.h>
#include "blockchain.h"
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

    BlockChainServer*
    BlockChain::GetServerInstance(){
        static BlockChainServer instance;
        return &instance;
    }

    static void*
    BlockChainServerThread(void* data){
        char* result = (char*) malloc(sizeof(char*) * 128);
        BlockChainServer* server = (BlockChainServer*) data;
        int rc;
        if((rc = server->Start()) < 0){
            strcpy(result, uv_strerror(rc));
        }
        pthread_exit(result);
    }

    void BlockChain::StartServer(int port){
        if(BlockChain::GetServerInstance()->IsRunning()) return;
        BlockChain::GetServerInstance()->SetPort(port);
        pthread_create(&server_thread_, NULL, &BlockChainServerThread, BlockChain::GetServerInstance());
    }

    void BlockChain::StopServer(){
        BlockChain::GetServerInstance()->Shutdown();
        WaitForServerShutdown();
    }

    void BlockChain::WaitForServerShutdown(){
        void* result;
        if(pthread_join(server_thread_, &result) != 0){
            std::cerr << "Unable to join server thread" << std::endl;
            return;
        }
        printf("Server shutdown with: %s\n", result);
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
            LOG(INFO) << "Loading BlockChain from '" << root << "'...";
            SetState(BlockChainState::LoadState(root));
            if(!UnclaimedTransactionPool::LoadUnclaimedTransactionPool(GetState()->GetUnclaimedTransactionPoolFile())){
                std::cout << "Cannot load unclaimed transaction pool" << std::endl;
                return false;
            }

            int height = GetHeight();
            std::string blk_filename = GetBlockDataFile(height);
            if(!FileExists(blk_filename)){
                std::cout << "Cannot load block: " << height << std::endl;
                return false;
            }
            std::cout << "Loading block " << height << " from " << blk_filename << std::endl;
            Block* nblock = Block::Load(blk_filename);
            if(!AppendGenesis(nblock)){
                std::cout << "Cannot append block" << std::endl;
                return false;
            }
            return Save();
        }
        LOG(WARNING) << "Cannot load BlockChain from path '" << root << "'; Creating genesis block...";
        LOG(WARNING) << "*** This functionality will be removed in future versions";
        LOG(WARNING) << "*** Genesis contains 128 base transactions for initialization";
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
        BlockChainNode* current = head_;
        while(current){
            current->GetBlock()->Write(GetBlockDataFile(current->GetHeight()));
            current = current->parent_;
        }
        return true;
    }

    Block* BlockChain::GetHead(){
        pthread_rwlock_rdlock(&rwlock_);
        Block* head = head_->GetBlock();
        pthread_rwlock_unlock(&rwlock_);
        return head;
    }

    bool BlockChain::Append(Token::Block* block){
        pthread_rwlock_wrlock(&rwlock_);
        if(nodes_.find(block->GetHash()) != nodes_.end()){
            std::cout << "Duplicate block found" << std::endl;
            pthread_rwlock_unlock(&rwlock_);
            return false;
        }
        if(block->GetHeight() == 0) return AppendGenesis(block);
        auto parent_node = nodes_.find(block->GetPreviousHash());
        if(parent_node == nodes_.end()){
            std::cout << "Cannot find parent" << std::endl;
            pthread_rwlock_unlock(&rwlock_);
            return false;
        }
        BlockChainNode* parent = GetBlockParent(block);

        BlockValidator validator;
        block->Accept(&validator);
        std::vector<Transaction*> valid = validator.GetValidTransactions();
        std::vector<Transaction*> invalid = validator.GetInvalidTransactions();
        if(valid.size() != block->GetNumberOfTransactions()){
            std::cout << "Not all transactions valid" << std::endl;
            pthread_rwlock_unlock(&rwlock_);
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
        Save();
        Message m(Message::Type::kBlockMessage, block->GetAsMessage());
        BlockChain::GetServerInstance()->Broadcast(nullptr, &m);
        pthread_rwlock_unlock(&rwlock_);
        return true;
    }

    Block* BlockChain::CreateBlock(){
        Block* parent = GetHead();
        return new Block(parent);
    }
}