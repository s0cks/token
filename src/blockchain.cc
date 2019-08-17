#include <sstream>
#include <glog/logging.h>
#include "blockchain.h"
#include "block_validator.h"
#include "block_resolver.h"

namespace Token{
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

    void BlockChain::SetGenesisHash(const std::string& hash){
        leveldb::WriteOptions writeOpts;
        if(!GetState()->Put(writeOpts, "GenesisHash", hash).ok()){
            LOG(ERROR) << "couldn't set genesis hash to: " << hash;
            return;
        }
    }

    void BlockChain::RegisterBlock(const std::string& hash, int height){
        leveldb::WriteOptions writeOpts;
        if(!GetState()->Put(writeOpts, hash, GetBlockDataFile(height)).ok()){
            LOG(ERROR) << "couldn't register block #" << height;
            return;
        }
    }

    std::string BlockChain::GetGenesisHash() const{
        std::string hash;
        leveldb::ReadOptions readOpts;
        if(!GetState()->Get(readOpts, "GenesisHash", &hash).ok()){
            LOG(ERROR) << "couldn't get genesis hash";
            return "";
        }
        return hash;
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

    int BlockChain::GetBlockHeightFromHash(const std::string &hash) const{
        std::string height;
        leveldb::ReadOptions readOpts;
        if(!GetState()->Get(readOpts, hash, &height).ok()){
            LOG(WARNING) << "couldn't find block '" << hash << "' in block list";
            return -1;
        }
        return atoi(height.c_str());
    }

    Block* BlockChain::CreateBlock(){
        Block* parent = GetHead();
        return new Block(parent);
    }

    std::string BlockChain::GetBlockDataFile(int height) const{
        std::stringstream stream;
        stream << root_ << "/blk" << height << ".dat";
        return stream.str();
    }

    Block* BlockChain::LoadBlock(int height) const{
        std::string blkf = GetBlockDataFile(height);
        if(!FileExists(blkf)){
            LOG(WARNING) << "block file '" << blkf << "' doesn't exist";
            return nullptr;
        }
        LOG(INFO) << "loading block#" << height << " from " << blkf << "....";
        return Block::Load(blkf);
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
            return SetHead(head);
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
        return Append(genesis);
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

    void BlockChain::Node::Accept(Token::BlockChainVisitor* vis){
        vis->Visit(GetBlock());
        for(auto& it : children_){
            it->Accept(vis);
        }
    }

    void BlockChain::Accept(Token::BlockChainVisitor* vis) const{
        if(HasHead()) GetHeadNode()->Accept(vis); //TODO: Fixme; Blockchain can possibly not have head when loading
    }

#define READ_LOCK pthread_rwlock_rdlock(&rwlock_)
#define WRITE_LOCK pthread_rwlock_wrlock(&rwlock_)
#define UNLOCK pthread_rwlock_unlock(&rwlock_)

    Block* BlockChain::GetHead(){
        READ_LOCK;
        Block* head = GetHeadNode()->GetBlock();
        UNLOCK;
        return head;
    }

    Block* BlockChain::GetBlockFromHash(const std::string& hash) const{
        BlockResolver resolver(hash);
        Accept(&resolver);
        if(resolver.HasResult()) return resolver.GetResult();
        //TODO: Fixme block possibly not loaded; Post-genesis Pre-<HEAD>
        int height;
        if((height = GetBlockHeightFromHash(hash)) < 0){
            LOG(ERROR) << "block '" << hash << "' not registered";
            return nullptr;
        }
        return LoadBlock(height);
    }

    Block* BlockChain::GetGenesis() const{
        return GetBlockFromHash(GetGenesisHash());
    }

    bool BlockChain::HasBlock(const std::string& hash) const{
        if(!HasHead()) return false;
        BlockResolver resolver(hash);
        Accept(&resolver);
        return resolver.HasResult();
    }

    bool BlockChain::Append(Token::Block* block){
        WRITE_LOCK;
        LOG(INFO) << "appending block: " << block->GetHash();
        if(HasBlock(block->GetHash())){
            LOG(ERROR) << "duplicate block found for: " << block->GetHash();
            UNLOCK;
            return false;
        }

        if(block->GetHeight() == 0){
            Transaction* cb = block->GetCoinbaseTransaction();
            std::string hash = cb->GetHash();
            for(int i = 0; i < cb->GetNumberOfOutputs(); i++){
                UnclaimedTransaction utxo(cb->GetHash(), i, cb->GetOutputAt(i));
                if(!UnclaimedTransactionPool::GetInstance()->AddUnclaimedTransaction(&utxo)){
                    LOG(ERROR) << "cannot append new unclaimed transaction: " << utxo.GetHash();
                    LOG(WARNING) << "*** Unclaimed Transaction: ";
                    LOG(WARNING) << "***   + Input: " << utxo.GetTransactionHash() << "[" << utxo.GetIndex() << "]";
                    LOG(WARNING) << "***   + Output: " << utxo.GetToken() << "(" << utxo.GetUser() << ")";
                    UNLOCK;
                    return false;
                }
            }
            SetGenesisHash(block->GetHash());
        } else if(block->GetHeight() > 0){
            Block* parent;
            if(!(parent = GetBlockFromHash(block->GetPreviousHash()))){
                LOG(ERROR) << "cannot find parent block: " << block->GetPreviousHash();
                UNLOCK;
                return false;
            }

            //TODO: Migrate validation logic
            BlockValidator validator;
            block->Accept(&validator);
            std::vector<Transaction*> valid = validator.GetValidTransactions();
            std::vector<Transaction*> invalid = validator.GetInvalidTransactions();
            if(valid.size() != block->GetNumberOfTransactions()){
                LOG(ERROR) << "block '" << block->GetHash() << "' is invalid";
                UNLOCK;
                return false;
            }
        } else{
            LOG(ERROR) << "invalid block height of " << block->GetHeight();
            UNLOCK;
            return false;
        }

        if(!SetHead(block)){
            LOG(ERROR) << "couldn't set block '" << block->GetHash() << "' to <HEAD>";
            UNLOCK;
            return false;
        }

        if(!SaveChain()){
            LOG(ERROR) << "couldn't save blockchain";
            UNLOCK;
            return false;
        }

        LOG(INFO) << "new <HEAD>: " << block->GetHash();
        //TODO: Migrate BroadCast logic
        Message m(Message::Type::kBlockMessage, block->GetAsMessage());
        BlockChain::GetServerInstance()->Broadcast(nullptr, &m);
        UNLOCK;
        return true;
    }

    bool BlockChain::SetHead(Token::Block* block){
        RegisterBlock(block->GetHash(), block->GetHeight());
        SetHeight(block->GetHeight());
        head_ = new Node(GetHeadNode(), block);
        return true;
    }

    bool BlockChain::SaveChain(){
        Node* current = GetHeadNode();
        while(current != nullptr && current->GetBlock() != nullptr){
            current->GetBlock()->Write(GetBlockDataFile(current->GetBlock()->GetHeight()));
            current = current->GetParent();
        }
        return true;
    }
}