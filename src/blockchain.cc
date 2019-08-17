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
        SetHeight(genesis->GetHeight());
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

    void BlockChain::SetHeight(int height){
        std::stringstream value;
        value << height;
        leveldb::WriteOptions writeOpts;
        if(!GetState()->Put(writeOpts, "Height", value.str()).ok()){
            LOG(ERROR) << "Couldn't set BlockChain's height in state to " << value.str();
            return;
        }
    }

    int BlockChain::GetHeight() const{
        std::string height;
        leveldb::ReadOptions readOpts;
        if(!GetState()->Get(readOpts, "Height", &height).ok()){
            LOG(ERROR) << "Couldn't get BlockChain's height in state";
            return -1;
        }
        return atoi(height.c_str());
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
            SetHeight(current->GetHeight());
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
        SaveChain();
        Message m(Message::Type::kBlockMessage, block->GetAsMessage());
        BlockChain::GetServerInstance()->Broadcast(nullptr, &m);
        pthread_rwlock_unlock(&rwlock_);
        return true;
    }

    Block* BlockChain::CreateBlock(){
        Block* parent = GetHead();
        return new Block(parent);
    }

    std::string BlockChain::GetBlockDataFile(int height){
        std::stringstream stream;
        stream << root_ << "/blk" << height << ".dat";
        return stream.str();
    }

    Block* BlockChain::LoadBlock(int height){
        std::string blkf = GetBlockDataFile(height);
        if(!FileExists(blkf)){
            LOG(WARNING) << "block file '" << blkf << "' doesn't exist";
            return nullptr;
        }
        return Block::Load(blkf);
    }

    bool BlockChain::SaveChain(){
        BlockChainNode* current = head_;
        while(current){
            current->GetBlock()->Write(GetBlockDataFile(current->GetHeight()));
            current = current->parent_;
        }
        return true;
    }

    bool BlockChain::InitializeChainHead(){
        int height = GetHeight();
        if(height != -1){
            Block* head;
            if((head = LoadBlock(height)) == nullptr){
                LOG(ERROR) << "cannot load head from height: " << height;
                LOG(ERROR) << "*** Fixme";
                return false;
            }
            if(!AppendGenesis(head)){
                LOG(ERROR) << "cannot append head as genesis";
                LOG(ERROR) << "*** Fixme";
                return false;
            }
            return SaveChain();
        }
        LOG(WARNING) << "Creating genesis block...";
        LOG(WARNING) << "*** This functionality will be removed in future versions";
        LOG(WARNING) << "*** Genesis contains 128 base transactions for initialization";
        Block* genesis = new Block(true);
        Transaction* cbtx = genesis->CreateTransaction();
        for(int i = 0; i < 128; i++){
            std::stringstream stream;
            stream << "Token" << i;
            cbtx->AddOutput(stream.str(), "TestUser");
        }
        if(!AppendGenesis(genesis)){
            LOG(ERROR) << "cannot append head as genesis";
            LOG(ERROR) << "*** Fixme";
            return false;
        }
        return SaveChain();
    }

    bool BlockChain::InitializeChainState(const std::string& root){
        root_ = root;
        leveldb::Options options;
        options.create_if_missing = true;
        if(!leveldb::DB::Open(options, (root + "/state"), &state_).ok()){
            LOG(ERROR) << "couldn't load chain state from '" << (root + "/state") << "'";
            return false;
        }
        LOG(INFO) << "loaded chain state from '" << (root + "/state") << "'";
        return true;
    }

    bool BlockChain::Initialize(const std::string& path){
        if(!BlockChain::GetInstance()->InitializeChainState(path)){
            return false;
        }
        return BlockChain::GetInstance()->InitializeChainHead();
    }
}