#include <sstream>
#include <glog/logging.h>

#include "allocator.h"
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

    std::string BlockChain::GetBlockLocation(uint32_t height){
        std::stringstream stream;
        stream << GetRootDirectory() << "/blk" << height << ".dat";
        return stream.str();
    }

    std::string BlockChain::GetBlockLocation(Token::Block* block){
        return GetBlockLocation(block->GetHeight());
    }

    std::string BlockChain::GetBlockLocation(const std::string& hash){
        std::string location;
        leveldb::ReadOptions readOpts;
        if(!GetState()->Get(readOpts, hash, &location).ok()){
            LOG(ERROR) << "couldn't get block location for: " << hash;
            return "";
        }
        return location;
    }

    bool BlockChain::SetBlockLocation(Token::Block *block, const std::string &filename){
        leveldb::WriteOptions writeOpts;
        if(!GetState()->Put(writeOpts, block->GetHash(), filename).ok()){
            LOG(ERROR) << "couldn't set block location for block '" << block->GetHash() << "' to: " << filename;
            return false;
        }
        return true;
    }

    bool BlockChain::SetHeight(int height){
        std::stringstream value;
        value << height;
        leveldb::WriteOptions writeOpts;
        if(!GetState()->Put(writeOpts, "ChainHeight", value.str()).ok()){
            LOG(ERROR) << "Couldn't set BlockChain's height in state to " << value.str();
            return false;
        }
        return true;
    }

    BlockChain::Node* BlockChain::GetNodeAt(uint32_t height){
        if(height > GetHeight()) return nullptr;
        if(height == GetHeight()) return GetInstance()->head_;

        int idx = GetHeight();
        Node* n = GetNodeAt(GetHeight());
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

    uint32_t BlockChain::GetHeight(){
        std::string height;
        leveldb::ReadOptions readOpts;
        if(!GetState()->Get(readOpts, "ChainHeight", &height).ok()){
            LOG(ERROR) << "couldn't get chain height from state";
            return -1;
        }
        return static_cast<uint32_t>(atoi(height.c_str()));
    }

    Block* BlockChain::CreateBlock(){
        Block* parent = GetHead();
        return new Block(parent);
    }

    bool BlockChain::InitializeChainHead(){
        int height = GetHeight();
        if(height != -1){
            Block* head;

            if((head = BlockChain::GetInstance()->LoadBlock(height)) == nullptr){
                LOG(ERROR) << "cannot load head from height: " << height;
                LOG(ERROR) << "*** Fixme";
                return false;
            }
            return SetHead(head);
        }
        LOG(WARNING) << "Creating genesis block...";
        LOG(WARNING) << "*** This functionality will be removed in future versions";
        LOG(WARNING) << "*** Genesis contains 128 base transactions for initialization";
        Block* genesis = new Block();
        Transaction* cbtx = genesis->CreateTransaction();
        for(int i = 0; i < 128; i++){
            std::stringstream stream;
            stream << "Token" << i;
            cbtx->AddOutput(stream.str(), "TestUser");
        }
        return AppendBlock(genesis);
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

    Block* BlockChain::GetBlock(uint32_t height){
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

    void BlockChain::Accept(Token::BlockChainVisitor* vis){
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

#define READ_LOCK pthread_rwlock_rdlock(&GetInstance()->rwlock_)
#define WRITE_LOCK pthread_rwlock_wrlock(&GetInstance()->rwlock_)
#define UNLOCK pthread_rwlock_unlock(&GetInstance()->rwlock_)

    Block* BlockChain::GetHead(){
        READ_LOCK;
        Block* head = GetNodeAt(GetHeight())->GetBlock();
        UNLOCK;
        return head;
    }

    Block* BlockChain::GetBlock(const std::string& hash){
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

    Block* BlockChain::GetGenesis(){
        return GetBlock(0);
    }

    bool BlockChain::ContainsBlock(const std::string& hash){
        Node* n = GetNodeAt(GetHeight());
        while(n != nullptr){
            if(n->GetBlock()->GetHash() == hash){
                return true;
            }
            n = n->GetParent();
        }
        return false;
    }

    bool BlockChain::ContainsBlock(Token::Block* block){
        Node* n = GetNodeAt(GetHeight());
        while(n != nullptr){
            if(n->GetBlock() == block){
                return true;
            }
            n = n->GetParent();
        }
        return false;
    }

    bool BlockChain::AppendBlock(Token::Block* block){
        WRITE_LOCK;
        LOG(INFO) << "appending block: " << block->GetHash();
        if(ContainsBlock(block)){
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
        } else if(block->GetHeight() > 0){
            Block* parent;
            if(!(parent = GetBlock(block->GetPreviousHash()))){
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
                LOG(ERROR) << "block information:";
                LOG(ERROR) << (*block);
                UNLOCK;
                return false;
            }
        } else{
            LOG(ERROR) << "invalid block height of " << block->GetHeight();
            UNLOCK;
            return false;
        }

        if(!SaveBlock(block)){
            LOG(ERROR) << "couldn't save block: " << block->GetHash();
            UNLOCK;
            return false;
        }

        if(!GetInstance()->SetHead(block)){
            LOG(ERROR) << "couldn't set <HEAD> to: " << block->GetHash();
            UNLOCK;
            return false;
        }

        //TODO: Migrate BroadCast logic
        // Message m(Message::Type::kBlockMessage, block->GetAsMessage());
        // BlockChain::GetServerInstance()->Broadcast(nullptr, &m);

        LOG(INFO) << "new <HEAD>: " << block->GetHash();
        UNLOCK;
        return true;
    }

    std::string BlockChain::GetRootDirectory(){
        return GetInstance()->root_;
    }

    bool BlockChain::SaveBlock(Token::Block* block){
        std::string blkfilename = GetBlockFile(block);
        if(FileExists(blkfilename)){
            LOG(WARNING) << "block already written, won't overwrite block";
            return false;
        }
        LOG(INFO) << "writing block '" << block->GetHash() << "' to file: " << blkfilename << "...";
        block->Write(blkfilename);
        return GetInstance()->SetBlockLocation(block, blkfilename);
    }

    bool BlockChain::SetHead(Token::Block* block){
        //TODO: Add reference to block
        //TODO: Remove reference to <HEAD>
        GetInstance()->head_ = new Node(head_, block);
        return SetHeight(block->GetHeight());
    }

    bool BlockChain::SetHead(const std::string& hash){
        LocalBlockResolver resolver(hash);
        if(!(resolver.Resolve() && resolver.HasResult())){
            LOG(ERROR) << "couldn't find block: " << hash;
            return false;
        }
        return SetHead(resolver.GetResult());
    }

    bool BlockChain::HasHead(){
        return GetHeight() >= 0;
    }

    Block* BlockChain::LoadBlock(const std::string& hash){
        return new Block(GetInstance()->GetBlockLocation(hash));
    }

    Block* BlockChain::LoadBlock(uint32_t height){
        return new Block(GetInstance()->GetBlockLocation(height));
    }
}