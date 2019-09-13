#include <sstream>
#include <glog/logging.h>
#include "blockchain.h"
#include "block_validator.h"
#include "block_resolver.h"
#include "server.h"

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

    BlockChain::Node* BlockChain::GetNodeAt(int height) const{
        if(height > GetHeight()) return nullptr;
        if(height == GetHeight()) return GetHeadNode();

        int idx = GetHeight();
        Node* n = GetHeadNode();
        while(idx != height && n != nullptr){
            idx--;
            n = n->GetParent();
        }
        return n;
    }

    int BlockChain::GetPeerCount(){
        leveldb::ReadOptions readOpts;
        std::string result;
        if(!GetState()->Get(readOpts, "PeerCount", &result).ok()){
            LOG(ERROR) << "couldn't get 'PeerCount' field";
            return -1;
        }
    }

    void BlockChain::SetPeerCount(int n){
        leveldb::WriteOptions writeOpts;
        std::stringstream val;
        val << n;
        if(!GetState()->Put(writeOpts, "PeerCount", val.str()).ok()){
            LOG(ERROR) << "couldn't set 'PeerCount' field";
        }
    }

    void BlockChain::RegisterPeer(Token::PeerClient* p){
        if(IsPeerRegistered(p)) return;

        int pidx = GetPeerCount() + 1;

        std::stringstream k;
        k << "peer_" << pidx;

        std::stringstream v;
        v << p->GetAddress() << ":" << p->GetPort();

        leveldb::WriteOptions writeOpts;
        if(!GetState()->Put(writeOpts, k.str(), v.str()).ok()){
            LOG(ERROR) << "couldn't register peer #" << pidx << " := " << v.str();
        }
    }

    std::string BlockChain::GetPeer(int n){
        if(n > GetPeerCount()) return "";
        std::stringstream k;
        k << "peer_" << n;

        std::string result;
        leveldb::ReadOptions readOpts;
        if(!GetState()->Get(readOpts, k.str(), &result).ok()){
            LOG(ERROR) << "couldn't get peer @ " << n;
            return "";
        }
        return result;
    }

    bool BlockChain::IsPeerRegistered(Token::PeerClient* p){
        std::stringstream target;
        target << p->GetAddress() << ":" << p->GetPort();
        int pcount = GetPeerCount();
        for(int i = 0; i < pcount; i++){
            std::string peer = GetPeer(i);
            if(target.str() == peer){
                return true;
            }
        }
        return false;
    }

    void BlockChain::DeregisterPeer(Token::PeerClient* p){
        //TODO: Implement
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
            LOG(ERROR) << "error initializing chain state";
            return false;
        }
        return BlockChain::GetInstance()->InitializeChainHead();
    }

    void BlockChain::Node::Accept(Token::BlockChainVisitor* vis){
        vis->Visit(GetBlock());
    }

    Block* BlockChain::GetBlockAt(int height) const{
        {
            // Search the in memory block chain first
            LOG(INFO) << "searching in-memory for block: " << height;
            BlockChainResolver resolver(height);
            resolver.Resolve();
            if(resolver.HasResult()){
                LOG(INFO) << "block " << height << " found!";
                return resolver.GetResult();
            }
            LOG(INFO) << "couldn't find block " << height << " in-memory, attempting to find it in the filesystem";
        }

        {
            // Now search the file system for the block
            LOG(INFO) << "searching the file system for block: " << height;
            LocalBlockResolver resolver(height);
            resolver.Resolve();
            if(resolver.HasResult()){
                LOG(INFO) << "block " << height << " found!";
                return resolver.GetResult();
            }
            LOG(INFO) << "couldn't find block " << height << " in the file system, attempting to find from the peers";
        }

        {
            // Now search peers for the block
            LOG(INFO) << "searching peers for block: " << height;
            std::vector<PeerClient*> peers;
            if(!BlockChainServer::GetPeerList(peers)){
                LOG(ERROR) << "couldn't get peer list";
                return nullptr;
            }
            int idx = 0;
            for(auto& it : peers){
                PeerBlockResolver resolver(it, height);
                resolver.Resolve();
                if(resolver.HasResult()){
                    LOG(INFO) << "block " << height << " found!";
                    return resolver.GetResult();
                }
                LOG(INFO) << "couldn't find block " << height << " in peer " << ((idx++) + 1) << "/" << peers.size();
            }
            LOG(INFO) << "couldn't find block " << height << " from peer list";
        }
        LOG(ERROR) << "couldn't find block: " << height;
        return nullptr;
    }

    void BlockChain::Accept(Token::BlockChainVisitor* vis) const{
        int idx = GetHeight();
        do{
            Node* n = GetNodeAt(idx);
            Block* blk;
            if(n){
                blk = n->GetBlock();
            } else{
                blk = LoadBlock(idx);
            }
            if(blk){
                vis->Visit(blk);
                if(blk->GetHeight() == 0) return; // Visited last block in chain!
                idx--;
            } else{
                LOG(ERROR) << "block @" << idx << " doesn't exist locally";
            }
        } while(true);
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
        {
            // Search the in memory block chain first
            LOG(INFO) << "searching in-memory for block: " << hash;
            BlockChainResolver resolver(hash);
            resolver.Resolve();
            if(resolver.HasResult()){
                LOG(INFO) << "block " << hash << " found!";
                return resolver.GetResult();
            }
            LOG(INFO) << "couldn't find block " << hash << " in-memory, attempting to find it in the filesystem";
        }

        {
            // Now search the file system for the block
            LOG(INFO) << "searching the file system for block: " << hash;
            LocalBlockResolver resolver(hash);
            resolver.Resolve();
            if(resolver.HasResult()){
                LOG(INFO) << "block " << hash << " found!";
                return resolver.GetResult();
            }
            LOG(INFO) << "couldn't find block " << hash << " in the file system, attempting to find from the peers";
        }

        {
            // Now search peers for the block
            LOG(INFO) << "searching peers for block: " << hash;
            std::vector<PeerClient*> peers;
            if(!BlockChainServer::GetPeerList(peers)){
                LOG(ERROR) << "couldn't get peer list";
                return nullptr;
            }
            int idx = 0;
            for(auto& it : peers){
                PeerBlockResolver resolver(it, hash);
                resolver.Resolve();
                if(resolver.HasResult()){
                    LOG(INFO) << "block " << hash << " found!";
                    return resolver.GetResult();
                }
                LOG(INFO) << "couldn't find block " << hash << " in peer " << ((idx++) + 1) << "/" << peers.size();
            }
            LOG(INFO) << "couldn't find block " << hash << " from peer list";
        }
        LOG(ERROR) << "couldn't find block: " << hash;
        return nullptr;
    }

    Block* BlockChain::GetGenesis() const{
        return GetBlockFromHash(GetGenesisHash());
    }

    bool BlockChain::HasBlock(const std::string& hash) const{
        if(!HasHead()) return false;
        LocalBlockResolver resolver(hash);
        resolver.Resolve();
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

            LOG(INFO) << "resolved parent: " << parent->GetHash();

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
        // Message m(Message::Type::kBlockMessage, block->GetAsMessage());
        // BlockChain::GetServerInstance()->Broadcast(nullptr, &m);
        UNLOCK;
        return true;
    }

    bool BlockChain::SetHead(const std::string &hash){
        SaveChain();
        delete head_;
        uint32_t height = GetBlockHeightFromHash(hash);
        if(height >= 0){
            SetHead(LoadBlock(height));
        }
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

    bool BlockChain::GetBlockList(std::vector<std::string>& blocks){
        int height = GetInstance()->GetHeight();
        do{
            Block* blk = GetInstance()->LoadBlock(height);
            if(blk == nullptr){
                return false;
            }
            blocks.push_back(blk->GetHash());
            delete blk;
            height--;
            if(height < 0){
                break;
            }
        }while(true);
        return true;
    }

    bool BlockChain::Save(Token::Block *block){
        block->Write(GetInstance()->GetBlockDataFile(block->GetHeight()));
    }
}