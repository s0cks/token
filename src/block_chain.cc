#include <sstream>
#include <glog/logging.h>

#include "common.h"
#include "block_chain.h"
#include "block_miner.h"

namespace Token{
    BlockChain::BlockChain():
        IndexManagedPool(FLAGS_path + "/blocks"),
        genesis_(nullptr),
        head_(nullptr){}

    BlockChain*
    BlockChain::GetInstance(){
        static BlockChain instance;
        return &instance;
    }

    uint64_t BlockChain::GetHeight(){
        //TODO: rwlock
        return GetInstance()->GetHeadNode()->GetHeight();
    }

    uint256_t BlockChain::GetHead(){
        //TODO: rwlock
        return GetInstance()->GetHeadNode()->GetHash();
    }

    uint256_t BlockChain::GetGenesis(){
        //TODO: rwlock
        return GetInstance()->GetGenesisNode()->GetHash();
    }

    uint256_t BlockChain::GetHeadFromIndex(){
        leveldb::ReadOptions readOpts;
        std::string key = "<HEAD>";
        std::string value;
        if(!GetIndex()->Get(readOpts, key, &value).ok()) return uint256_t();
        return HashFromHexString(value);
    }

    bool BlockChain::HasHeadInIndex(){
        leveldb::ReadOptions readOpts;
        std::string key = "<HEAD>";
        std::string value;
        if(!GetIndex()->Get(readOpts, key, &value).ok()) return false;
        return value.length() > 0;
    }

    bool BlockChain::SetHeadInIndex(const Token::uint256_t& hash){
        leveldb::WriteOptions writeOpts;
        std::string key = "<HEAD>";
        std::string value = HexString(hash);
        return GetIndex()->Put(writeOpts, key, value).ok();
    }

    static Block*
    CreateGenesis(){
        Transaction coinbase(0, 0);
        coinbase << Output(&coinbase, "TestUser", "TestToken");
        return new Block(0, uint256_t(), { coinbase }, 0);
    }

    bool BlockChain::Initialize(){
        BlockChain* chain = GetInstance();
        if(!FileExists(chain->GetRoot())){
            if(!CreateDirectory(chain->GetRoot())) return false;
        }
        if(!chain->InitializeIndex()) return false;;
        if(!chain->HasHeadInIndex()){
            Block* genesis = CreateGenesis();
            uint256_t hash = genesis->GetHash();
            std::string filename = chain->GetPath(hash);
            if(!chain->SaveBlock(filename, genesis)) return false;
            if(!chain->SetHeadInIndex(hash)) return false;
            return chain->RegisterPath(hash, filename);
        }

        uint256_t hash = chain->GetHeadFromIndex();
        Block* block = GetBlock(hash);
        BlockNode* node = new BlockNode(block);
        BlockNode* head = node;
        BlockNode* parent = node;
        do{
            LOG(INFO) << "loading block: " << hash << "...";
            block = GetBlock(hash);
            node = new BlockNode(block);
            chain->nodes_.insert(std::make_pair(hash, node));
            parent->AddChild(node);
            parent = node;
            hash = node->GetPreviousHash();
        } while(!hash.IsNull());
        chain->head_ = head;
        chain->genesis_ = node;
        return true;
    }

    bool BlockChain::LoadBlock(const std::string& filename, Token::Block* block){
        if(!FileExists(filename)) return false;
        std::fstream fd(filename, std::ios::in|std::ios::binary);
        Proto::BlockChain::Block raw;
        if(!raw.ParseFromIstream(&fd)) return false;
        (*block) = Block(raw);
        return true;
    }

    bool BlockChain::SaveBlock(const std::string& filename, Token::Block* block){
        if(FileExists(filename)) return false;
        std::fstream fd(filename, std::ios::out|std::ios::binary);
        Proto::BlockChain::Block raw;
        raw << (*block);
        return raw.SerializeToOstream(&fd);
    }

    BlockChain::BlockNode* BlockChain::GetHeadNode(){
        BlockChain* chain = GetInstance();
        //TODO: rwlock
        BlockNode* head = chain->head_;
        return head;
    }

    BlockChain::BlockNode* BlockChain::GetGenesisNode() {
        BlockChain* chain = GetInstance();
        //TODO: rwlock
        BlockNode* genesis = chain->genesis_;
        return genesis;
    }

    BlockChain::BlockNode* BlockChain::GetNode(const Token::uint256_t& hash){
        BlockChain* chain = GetInstance();
        //TODO: rwlock
        auto pos = chain->nodes_.find(hash);
        if(pos == chain->nodes_.end()) return nullptr;
        return pos->second;
    }

    Block* BlockChain::GetBlock(const uint256_t& hash){
        BlockChain* chain = GetInstance();
        //TODO: rwlock
        std::string filename = chain->GetPath(hash);
        if(!FileExists(filename)) return nullptr;
        Block* block = new Block();
        if(!chain->LoadBlock(filename, block)){
            delete block;
            return nullptr;
        }
        return block;
    }

    Transaction* BlockChain::GetTransaction(const uint256_t& hash){
        return nullptr; //TODO: implement
    }

    bool BlockChain::ContainsBlock(const uint256_t& hash){
        BlockChain* chain = GetInstance();
        //TODO rwlock
        std::string filename = chain->GetPath(hash);
        return FileExists(filename);
    }

    bool BlockChain::ContainsTransaction(const uint256_t& hash){
        return false; //TODO: implement
    }

    bool BlockChain::Append(Block* block){
        /*TODO Move to miner
        BlockChain* chain = GetInstance();
        uint256_t phash = block->GetPreviousHash();
        uint256_t hash = block->GetHash();

        if(BlockChain::ContainsBlock(hash)){
            LOG(ERROR) << "duplicate block found for: " << hash;
            return false;
        }

        if(block->IsGenesis() && !chain->IsInitialized() && !GetHead().IsNull()){
            LOG(ERROR) << "cannot append genesis block: " << block->GetHash();
            return false;
        }

        if(block->IsGenesis()){
            int i;
            for(i = 0; i < block->GetNumberOfTransactions(); i++){
                Transaction* tx;
                if(!(tx = block->GetTransaction(i))){
                    LOG(ERROR) << "couldn't get transaction #" << i << " in block: " << block->GetHash();
                    return false;
                }

                LOG(INFO) << "registering " << tx->GetNumberOfOutputs() << " unclaimed transactions...";
                int j;
                for(j = 0; j < tx->GetNumberOfOutputs(); j++) {
                    Output* out;
                    if(!(out = tx->GetOutput(j))){
                        LOG(WARNING) << "couldn't get output #" << j;
                        return false;
                    }

                    UnclaimedTransaction utxo(out);

                    if(!UnclaimedTransactionPool::PutUnclaimedTransaction(utxo)){
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
            Block* parent;
            if(!(parent = GetBlock(block->GetPreviousHash()))){
                LOG(ERROR) << "couldn't find parent block: " << block->GetPreviousHash();
                return false;
            }

            //TODO: Migrate validation logic
            BlockValidator validator;
            block->Accept(&validator);

            std::vector<Transaction*> valid = validator.GetValidTransactions();
            std::vector<Transaction*> invalid = validator.GetInvalidTransactions();
            if(valid.size() != block->GetNumberOfTransactions()){
                LOG(ERROR) << "block '" << block->GetHash() << "' is invalid";
                LOG(ERROR) << "block information:";
                return false;
            }
        }
        */
        //TODO: rwlock
        BlockChain* chain = GetInstance();
        uint256_t hash = block->GetHash();
        uint256_t phash = block->GetPreviousHash();

        std::string filename = chain->GetPath(hash);
        if(FileExists(filename)){
            LOG(WARNING) << "block data already exists";
            return false;
        }
        if(!SaveBlock(filename, block)) {
            LOG(INFO) << "couldn't save block: " << filename;
            return false;
        }
        if(!chain->RegisterPath(hash, filename)) {
            LOG(INFO) << "couldn't register path: " << hash;
            return false;
        }

        LOG(INFO) << "phash := " << phash;
        BlockNode* parent = GetNode(phash);
        BlockNode* node = new BlockNode(block);
        parent->AddChild(node);
        BlockNode* head = GetHeadNode();
        if(head->GetHeight() < block->GetHeight()) {
            chain->SetHeadInIndex(hash);
            head_ = node;
        }
        nodes_.insert(std::make_pair(hash, node));
        return true;
    }

    bool BlockChain::Accept(BlockChainVisitor* vis){
        if(!vis->VisitStart()) return false;
        uint256_t gHash = GetGenesis();
        uint256_t hash = GetHead();
        Block* block = nullptr;
        do{
            block = GetBlock(hash);
            if(!vis->VisitBlock(block)){
                delete block;
                return false;
            }

            hash = block->GetPreviousHash();
            delete block;
        } while(!hash.IsNull());
        return vis->VisitEnd();
    }

    class BlockChainMerkleTreeBuilder : public BlockChainVisitor,
                                        public BlockVisitor{
    private:
        std::vector<uint256_t> leaves_;
    public:
        BlockChainMerkleTreeBuilder():
            leaves_(){}
        ~BlockChainMerkleTreeBuilder(){}

        bool VisitBlock(Block* block){
            return block->Accept(this);
        }

        bool VisitTransaction(Transaction* tx){
            leaves_.push_back(tx->GetHash());
            return true;
        }

        bool BuildTree(){
            return BlockChain::Accept(this);
        }

        MerkleTree* GetTree(){
            return new MerkleTree(leaves_);
        }
    };

    MerkleTree* BlockChain::GetMerkleTree(){
        BlockChainMerkleTreeBuilder builder;
        if(!builder.BuildTree()) return nullptr;
        return builder.GetTree();
    }
}