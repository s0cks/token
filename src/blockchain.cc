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
        uint32_t height;
        if(!GetInstance()->GetReference("<HEAD>", &height)){
            LOG(ERROR) << "cannot get <HEAD>";
            return -1;
        }
        return height;
    }

    bool BlockChain::CreateGenesis(){
        //TODO: Remove
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

    bool BlockChain::LoadBlockChain(const std::string& path){
        uint32_t head = GetHeight();
        if(head == -1) return CreateGenesis(); //TODO: Remove
        uint32_t height = 0;
        while(height <= head){
            LOG(INFO) << "loading block #" << height << "...";
            Block* blk;
            if(!(blk = GetInstance()->LoadBlock(height))){
                LOG(ERROR) << "cannot load block from height: " << height;
                LOG(ERROR) << "*** fixme";
                return false;
            }
            if(!AppendNode(blk)){
                LOG(ERROR) << "cannot append block node from height: " << height;
                LOG(ERROR) << "*** fixme";
                return false;
            }
            height++;
        }
        return true;
    }

    bool BlockChain::InitializeChainState(const std::string& root){
        path_ = root;
        leveldb::Options options;
        options.create_if_missing = true;
        if(!leveldb::DB::Open(options, (root + "/state"), &state_).ok()){
            LOG(ERROR) << "couldn't load chain state from '" << (root + "/state") << "'";
            return false;
        }
        LOG(INFO) << "loaded chain state from '" << (root + "/state") << "'";
        return true;
    }

    static inline void
    Encode(const std::string& filename, CryptoPP::BufferedTransformation& bt){
        CryptoPP::FileSink file(filename.c_str());
        bt.CopyTo(file);
        file.MessageEnd();
    }

    static inline void
    EncodePrivateKey(const std::string& filename, const CryptoPP::RSA::PrivateKey& key){
        CryptoPP::ByteQueue queue;
        key.DEREncodePrivateKey(queue);
        Encode(filename, queue);
    }

    static inline void
    EncodePublicKey(const std::string& filename, const CryptoPP::RSA::PublicKey& key){
        CryptoPP::ByteQueue queue;
        key.DEREncodePublicKey(queue);
        Encode(filename, queue);
    }

    static inline void
    Decode(const std::string& filename, CryptoPP::BufferedTransformation& bt){
        CryptoPP::FileSource file(filename.c_str(), true);
        file.TransferTo(bt);
        bt.MessageEnd();
    }

    static inline void
    DecodePrivateKey(const std::string& filename, CryptoPP::RSA::PrivateKey& key){
        CryptoPP::ByteQueue queue;
        Decode(filename, queue);
        key.BERDecodePrivateKey(queue, false, queue.MaxRetrievable());
    }

    static inline void
    DecodePublicKey(const std::string& filename, CryptoPP::RSA::PublicKey& key){
        CryptoPP::ByteQueue queue;
        Decode(filename, queue);
        key.BERDecodePublicKey(queue, false, queue.MaxRetrievable());
    }

    bool BlockChain::GenerateChainKeys(const std::string &pubkey, const std::string &privkey){
        CryptoPP::AutoSeededRandomPool rng;
        CryptoPP::InvertibleRSAFunction params;
        params.GenerateRandomWithKeySize(rng, BlockChain::kKeypairSize);
        privkey_ = CryptoPP::RSA::PrivateKey(params);
        pubkey_ = CryptoPP::RSA::PublicKey(params);
        EncodePublicKey(pubkey, pubkey_);
        EncodePrivateKey(privkey, privkey_);
    }

    bool BlockChain::InitializeChainKeys(const std::string& path){
        std::string pubkey = (path + "/chain.pub");
        std::string privkey = (path + "/chain.priv");
        if(!FileExists(privkey)){
            LOG(INFO) << "generating chain keys";
            GenerateChainKeys(pubkey, privkey);
        } else{
            LOG(INFO) << "loading chain keys";
            DecodePublicKey(pubkey, pubkey_);
            DecodePrivateKey(privkey, privkey_);
        }
        return true;
    }

    static inline bool
    CreateDirectory(const std::string& path){
        return (mkdir(path.c_str(), S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH)) != -1;
    }

    bool BlockChain::Initialize(const std::string& path){
        LOG(INFO) << "initializing block chain in path: " << path << "...";
        if(!BlockChain::GetInstance()->InitializeChainKeys(path)){
            LOG(ERROR) << "error initializing chain keys";
            return false;
        }
        if(!BlockChain::GetInstance()->InitializeChainState(path)){
            LOG(ERROR) << "error initializing chain state";
            return false;
        }
        std::string blks = (path + "/blocks");
        if(!FileExists(blks)){
            LOG(WARNING) << "blocks cache doesn't exist, creating...";
            if(!CreateDirectory(blks)){
                LOG(ERROR) << "cannot create blocks cache directory";
                return false;
            }
        }
        LOG(INFO) << "loading block chain....";
        if(!BlockChain::GetInstance()->LoadBlockChain(path)){
            LOG(ERROR) << "error loading block chain";
            return false;
        }
        return true;
    }

    Block* BlockChain::GetBlock(uint32_t height){
        if(height > GetHeight() || height < 0) return nullptr;
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
        ChainNode* node = GetInstance()->GetGenesisNode();
        while(node){
            vis->Visit(node->GetBlock());
            node = node->GetNext();
        }
    }

#define READ_LOCK pthread_rwlock_rdlock(&GetInstance()->rwlock_)
#define WRITE_LOCK pthread_rwlock_wrlock(&GetInstance()->rwlock_)
#define UNLOCK pthread_rwlock_unlock(&GetInstance()->rwlock_)

    Block* BlockChain::GetHead(){
        READ_LOCK;
        Block* head = GetInstance()->GetHeadNode()->GetBlock();
        UNLOCK;
        return head;
    }

    Block* BlockChain::GetBlock(const std::string& hash){
        {
            // Search the in memory block chain first
            LOG(INFO) << "searching in-memory for block: " << hash;
            BlockChainResolver resolver(hash);
            if(resolver.Resolve()){
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
        READ_LOCK;
        Block* blk = GetInstance()->GetGenesisNode()->GetBlock();
        UNLOCK;
        return blk;
    }

    bool BlockChain::ContainsBlock(const std::string& hash){
        BlockChainResolver resolver(hash);
        return resolver.Resolve() && resolver.HasResult();
    }

    bool BlockChain::ContainsBlock(Token::Block* block){
        return ContainsBlock(block->GetHash());
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

        if(!GetInstance()->SetReference("<HEAD>", block->GetHeight())){
            LOG(ERROR) << "couldn't set <HEAD> reference";
            return false;
        }

        if(!GetInstance()->AppendNode(block)){
            LOG(ERROR) << "couldn't append node for new <HEAD> := " << block->GetHash();
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
        return GetInstance()->path_;
    }

    bool BlockChain::SaveBlock(Token::Block* block){
        std::string blkfilename = GetInstance()->GetBlockFile(block->GetHeight());
        if(FileExists(blkfilename)){
            LOG(WARNING) << "block already written, won't overwrite block";
            return false;
        }
        LOG(INFO) << "writing block '" << block->GetHash() << "' to file: " << blkfilename << "...";
        block->Write(blkfilename);
        return GetInstance()->SetReference(block->GetHash(), block->GetHeight());
    }

    bool BlockChain::HasHead(){
        return GetHeight() >= 0;
    }

    bool BlockChainPrinter::Visit(Token::Block* block){
        LOG(INFO) << "  - #" << block->GetHeight() << ": " << block->GetHash();
        return true;
    }

    void BlockChainPrinter::PrintBlockChain(){
        LOG(INFO) << "BlockChain (" << BlockChain::GetHeight() << " blocks):";
        BlockChainPrinter instance;
        BlockChain::Accept(&instance);
    }

    //TODO: v3

    bool BlockChain::SetReference(const std::string &ref, uint32_t height){
        std::stringstream val;
        val << height;
        leveldb::WriteOptions writeOpts;
        if(!GetState()->Put(writeOpts, ref, val.str()).ok()){
            LOG(ERROR) << "error setting reference '" << ref << "' to: " << height;
            return false;
        }
        return true;
    }

    bool BlockChain::GetReference(const std::string& ref, uint32_t* height){
        std::string val;
        leveldb::ReadOptions readOpts;
        if(!GetState()->Get(readOpts, ref, &val).ok()){
            LOG(ERROR) << "error getting reference to: " << ref;
            (*height) = -1;
            return false;
        }
        (*height) = static_cast<uint32_t>(atoll(val.c_str()));
        return true;
    }

    std::string BlockChain::GetBlockFile(uint32_t height){
        std::stringstream stream;
        stream << GetRootDirectory() << "/blocks";
        stream << "/blk" << height << ".dat";
        return stream.str();
    }

    std::string BlockChain::GetBlockFile(const std::string& hash){
        uint32_t height;
        if(!GetReference(hash, &height)){
            LOG(ERROR) << "cannot get height for block: " << hash;
            return "";
        }
        return GetBlockFile(height);
    }

    Block* BlockChain::LoadBlock(const std::string& hash){
        std::string filename = GetInstance()->GetBlockFile(hash);
        Block* blk = new Block();
        if(!blk->LoadBlockFromFile(filename)){
            LOG(ERROR) << "cannot load block '" << hash << "' from file: " << filename;
            return nullptr;
        }
        return blk;
    }

    Block* BlockChain::LoadBlock(uint32_t height){
        std::string filename = GetInstance()->GetBlockFile(height);
        Block* blk = new Block();
        if(!blk->LoadBlockFromFile(filename)){
            LOG(ERROR) << "cannot load block #" << height << " from file: " << filename;
            return nullptr;
        }
        return blk;
    }

    bool BlockChain::AppendNode(Token::Block* block){
        ChainNode* node = new ChainNode(block);
        ChainNode* head = GetInstance()->GetHeadNode();
        if(!head){
            GetInstance()->SetHeadNode(node);
            GetInstance()->SetGenesisNode(node);
            return true;
        }
        head->SetNext(node);
        node->SetPrevious(head);
        GetInstance()->SetHeadNode(node);
        return true;
    }
}