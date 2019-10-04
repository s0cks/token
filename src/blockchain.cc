#include <sstream>
#include <glog/logging.h>

#include "allocator.h"
#include "blockchain.h"
#include "block_validator.h"
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
        Transaction* cbtx = new Transaction();
        for(int i = 0; i < 128; i++){
            std::stringstream stream;
            stream << "Token" << i;
            cbtx->AddOutput(stream.str(), "TestUser");
        }
        genesis->AppendTransaction(cbtx); //TODO This needs to be here
        Allocator::AddReference(genesis);
        return AppendBlock(genesis);
    }

    bool BlockChain::LoadBlockChain(const std::string& path){
        uint32_t head = GetHeight();
        if(head == -1) return CreateGenesis(); //TODO: Remove
        uint32_t height = 0;
        while(height <= head){
            LOG(INFO) << "loading block #" << height << "...";
            Block* blk;
            if(!GetInstance()->LoadBlock(height, &blk)){
                LOG(ERROR) << "cannot load block from height: " << height;
                LOG(ERROR) << "*** fixme";
                return false;
            }
            Allocator::AddReference(blk);
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

    Block* BlockChain::GetGenesis(){
        READ_LOCK;
        Block* blk = GetInstance()->GetGenesisNode()->GetBlock();
        UNLOCK;
        return blk;
    }

    bool BlockChain::AppendBlock(Token::Block* block){
        WRITE_LOCK;
        LOG(INFO) << "appending block: " << block->GetHash();
        LOG(INFO) << "checking for duplicate block...";
        if(ContainsBlock(block->GetHash())){
            LOG(ERROR) << "duplicate block found for: " << block->GetHash();
            UNLOCK;
            return false;
        }

        if(block->IsGenesis() && HasHead()){
            LOG(ERROR) << "cannot append genesis block:";
            LOG(ERROR) << (*block);
            UNLOCK;
            return false;
        }

        if(block->IsGenesis()){
            LOG(INFO) << "registering genesis's " << block->GetNumberOfTransactions() << " transactions...";

            int i;
            for(i = 0; i < block->GetNumberOfTransactions(); i++){
                Transaction* tx = block->GetTransactionAt(i);
                Allocator::AddReference(tx);
                LOG(INFO) << "registering " << tx->GetNumberOfOutputs() << " unclaimed transactions...";
                int j;
                for(j = 0; j < tx->GetNumberOfOutputs(); j++) {
                    UnclaimedTransaction utxo(tx->GetHash(), j, tx->GetOutputAt(j));
                    if (!UnclaimedTransactionPool::GetInstance()->AddUnclaimedTransaction(&utxo)) {
                        LOG(WARNING) << "couldn't create new unclaimed transaction: " << utxo.GetHash();
                        LOG(WARNING) << "*** Unclaimed Transaction: ";
                        LOG(WARNING) << "***   + Input: " << utxo.GetTransactionHash() << "[" << utxo.GetIndex()
                                     << "]";
                        LOG(WARNING) << "***   + Output: " << utxo.GetToken() << "(" << utxo.GetUser() << ")";
                        return false;
                    }
                    LOG(INFO) << "added new unclaimed transaction: " << utxo.GetHash();
                }
            }
        } else{
            LOG(INFO) << "finding parent block";
            Block* parent;
            if(!(parent = GetBlock(block->GetPreviousHash()))){
                LOG(ERROR) << "cannot find parent block: " << block->GetPreviousHash();
                UNLOCK;
                return false;
            }
            LOG(INFO) << "found parent: " << parent->GetHash();

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
        }

        if(!GetInstance()->SaveBlock(block)){
            LOG(ERROR) << "couldn't save block: " << block->GetHash();
            UNLOCK;
            return false;
        }

        if(!GetInstance()->SetReference("<HEAD>", block->GetHeight())){
            LOG(ERROR) << "couldn't set <HEAD> reference";
            return false;
        }

        Allocator::AddReference(block);
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

    bool BlockChain::HasHead(){
        return GetInstance()->GetHeadNode() != nullptr;
    }

    bool BlockChainPrinter::Visit(Token::Block* block){
        LOG(INFO) << "  - #" << block->GetHeight() << ": " << block->GetHash();
        if(ShouldPrintInfo()){
            LOG(INFO) << " - Info:";
            LOG(INFO) << "\t" << (*block);
        }
        return true;
    }

    void BlockChainPrinter::PrintBlockChain(bool info){
        LOG(INFO) << "BlockChain (" << BlockChain::GetHeight() << " blocks):";
        BlockChainPrinter instance(info);
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

    bool BlockChain::LoadBlock(uint32_t height, Token::Block **result){
        if(height < 0 || height > GetHeight()){
            *result = nullptr;
            return false;
        }

        std::stringstream stream;
        stream << GetRootDirectory() << "/blocks/blk" << height << ".dat";
        (*result) = new Block();
        if(!(*result)->LoadBlockFromFile(stream.str())){
            LOG(ERROR) << "cannot load block #" << height;
            return false;
        }
        return true;
    }

    bool BlockChain::LoadBlock(const std::string &hash, Token::Block **result){
        uint32_t height;
        if(!GetReference(hash, &height)){
            LOG(ERROR) << "cannot find block: " << hash;
            *result = nullptr;
            return false;
        }
        return LoadBlock(height, result);
    }

    bool BlockChain::SaveBlock(Token::Block* block){
        std::stringstream filename;
        filename << GetRootDirectory() << "/blocks";
        filename << "/blk" << block->GetHeight() << ".dat";
        std::string blkfilename = filename.str();
        if(FileExists(blkfilename)){
            LOG(WARNING) << "block already written, won't overwrite block: " << blkfilename;
            return false;
        }
        LOG(INFO) << "writing block '" << block->GetHash() << "' to file: " << blkfilename << "...";
        block->Write(blkfilename);
        return GetInstance()->SetReference(block->GetHash(), block->GetHeight());
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

    ChainNode* BlockChain::GetNode(uint32_t height){
        if(height < 0 || height > GetHeight()) return nullptr;
        uint32_t idx = 0;
        ChainNode* c = GetGenesisNode();
        while(idx < height && c){
            idx++;
            c = c->GetNext();
        }
        return c;
    }

    Block* BlockChain::GetBlock(uint32_t height){
        if(height < 0 || height > GetHeight()) return nullptr;
        return GetInstance()->GetNode(height)->GetBlock();
    }

    Block* BlockChain::GetBlock(const std::string &hash){
        uint32_t height;
        if(!GetInstance()->GetReference(hash, &height)){
            LOG(ERROR) << "cannot find block: " << hash;
            return nullptr;
        }
        return GetInstance()->GetNode(height)->GetBlock();
    }

    bool BlockChain::ContainsBlock(const std::string &hash){
        uint32_t height;
        if(!GetInstance()->GetReference(hash, &height)){
            LOG(ERROR) << "cannot find block: " << hash;
            return false;
        }
        return height >= 0 && height <= GetHeight();
    }

    bool BlockChain::GetBlockList(std::vector<std::string>& blocks){
        ChainNode* n = GetInstance()->GetGenesisNode();
        while(n){
            blocks.push_back(n->GetHash());
            n = n->GetNext();
        }
        return blocks.size() == GetHeight();
    }

    bool BlockChain::GetBlockList(Token::Node::Messages::BlockList *blocks){
        ChainNode* n = GetInstance()->GetGenesisNode();
        while(n){
            Block* block = n->GetBlock();
            Messages::BlockHeader* header = blocks->add_blocks();
            header->set_height(block->GetHeight());
            header->set_hash(block->GetHash());
            header->set_previous_hash(block->GetPreviousHash());
            header->set_merkle_root(block->GetMerkleRoot());
            n = n->GetNext();
        }
        return (blocks->blocks_size() - 1) == GetHeight();
    }
}