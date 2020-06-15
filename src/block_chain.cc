#include <pthread.h>
#include <sstream>
#include <glog/logging.h>
#include "common.h"
#include "block_chain.h"
#include "block_miner.h"
#include "object.h"

namespace Token{
#define READ_LOCK pthread_rwlock_tryrdlock(&chain->rwlock_);
#define WRITE_LOCK pthread_rwlock_trywrlock(&chain->rwlock_);
#define UNLOCK pthread_rwlock_unlock(&chain->rwlock_);

    BlockChain::BlockChain():
        IndexManagedPool(FLAGS_path + "/data"),
        rwlock_(),
        nodes_(),
        blocks_(),
        genesis_(nullptr),
        head_(nullptr){
        pthread_rwlock_init(&rwlock_, NULL);
    }

    BlockChain*
    BlockChain::GetInstance(){
        static BlockChain instance;
        return &instance;
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

    static const uint32_t kNumberOfGenesisOutputs = 32;

    static Block*
    CreateGenesis(){
        Transaction::InputList cb_inputs;
        Transaction::OutputList cb_outputs;
        for(uint32_t idx = 0; idx < kNumberOfGenesisOutputs; idx++){
            std::string user = "TestUser";
            std::stringstream token;
            token << "TestToken" << idx;

            cb_outputs.push_back(std::unique_ptr<Output>(Output::NewInstance(user, token.str())));
        }
        std::vector<Transaction*> txs = {
            Transaction::NewInstance(0, cb_inputs, cb_outputs, 0)
        };
        return Block::NewInstance(0, uint256_t(), txs, 0);
    }

    bool BlockChain::Initialize(){
        BlockChain* chain = GetInstance();
        if(!FileExists(chain->GetRoot())){
            if(!CreateDirectory(chain->GetRoot())) return false;
        }
        if(!chain->InitializeIndex()) return false;;
        if(!chain->HasHeadInIndex()){
            Block* genesis;
            if(!(genesis = CreateGenesis())){
                LOG(ERROR) << "couldn't create genesis";
                return false;
            }

            uint256_t hash = genesis->GetHash();
            if(!chain->SaveObject(hash, genesis)) return false;
            BlockNode* node = new BlockNode(genesis);
            chain->head_ = chain->genesis_ = node;
            chain->blocks_.insert(std::make_pair(genesis->GetHeight(), node));
            chain->nodes_.insert(std::make_pair(hash, node));

            for(auto it : (*genesis)){
                uint32_t index = 0;
                for(auto out = it->outputs_begin(); out != it->outputs_end(); out++){
                    UnclaimedTransaction* utxo = UnclaimedTransaction::NewInstance(it->GetHash(), index++);
                    if(!UnclaimedTransactionPool::PutUnclaimedTransaction(utxo)){
                        LOG(WARNING) << "couldn't create new unclaimed transaction: " << utxo->GetHash();
                        UNLOCK;
                        return false;
                    }
                }
            }
            chain->SetHeadInIndex(hash);
            return true;
        }

        uint256_t hash = chain->GetHeadFromIndex();
        Block* block = nullptr;
        if(!(block = GetBlockData(hash))) return false;

        if(!Allocator::AddRoot(block)){
            LOG(WARNING) << "couldn't add block to roots!";
            return false;
        }

        BlockNode* node = new BlockNode(block);
        BlockNode* head = node;
        BlockNode* parent = node;
        do{
            if(!(block = GetBlockData(hash))){
                LOG(ERROR) << "cannot load block data!";
                return false;
            }

            if(!Allocator::AddRoot(block)){
                LOG(WARNING) << "couldn't add block to roots!";
                return false;
            }

            //TODO: LOG(INFO) << "loaded block: " << BlockHeader(block);
            node = new BlockNode(block);
            chain->blocks_.insert(std::make_pair(block->GetHeight(), node));
            chain->nodes_.insert(std::make_pair(hash, node));
            parent->AddChild(node);
            parent = node;
            hash = node->GetPreviousHash();
        } while(!hash.IsNull());
        chain->head_ = head;
        chain->genesis_ = node;
        return true;
    }

    BlockChain::BlockNode* BlockChain::GetHeadNode(){
        BlockChain* chain = GetInstance();
        READ_LOCK;
        BlockNode* node = chain->head_;
        UNLOCK;
        return node;
    }

    BlockChain::BlockNode* BlockChain::GetGenesisNode(){
        BlockChain* chain = GetInstance();
        BlockNode* node = chain->genesis_;
        return node;
    }

    BlockChain::BlockNode* BlockChain::GetNode(const uint256_t& hash){
        BlockChain* chain = GetInstance();
        auto pos = chain->nodes_.find(hash);
        if(pos == chain->nodes_.end()) return nullptr;
        return pos->second;
    }

    BlockChain::BlockNode* BlockChain::GetNode(uint32_t height){
        if(height < 0 || height > GetHeight()){
            LOG(WARNING) << "height too big!";
            return nullptr;
        }
        BlockChain* chain = GetInstance();
        auto pos = chain->blocks_.find(height);
        if(pos == chain->blocks_.end()) return nullptr;
        return pos->second;
    }

    Block* BlockChain::GetBlockData(const uint256_t& hash){
        BlockChain* chain = GetInstance();
        READ_LOCK;
        Block* block = chain->LoadObject(hash);
        UNLOCK;
        return block;
    }

    bool BlockChain::Append(Block* block){
        BlockChain* chain = GetInstance();
        WRITE_LOCK;
        uint256_t hash = block->GetHash();
        uint256_t phash = block->GetPreviousHash();

        if(chain->ContainsObject(hash)){
            LOG(ERROR) << "duplicate block found for: " << hash;
            UNLOCK;
            return false;
        }

        if(block->IsGenesis()){
            LOG(ERROR) << "cannot append genesis block: " << hash;
            UNLOCK;
            return false;
        }

        if(!BlockChain::ContainsBlock(phash)){
            LOG(ERROR) << "couldn't find parent: " << phash;
            UNLOCK;
            return false;
        }

        if(!chain->SaveObject(hash, block)){
            LOG(ERROR) << "couldn't save block: " << hash;
            UNLOCK;
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
        blocks_.insert(std::make_pair(block->GetHeight(), node));
        nodes_.insert(std::make_pair(hash, node));

        UNLOCK;
        LOG(INFO) << "appended block: " << hash;
        return true;
    }

    bool BlockChain::Accept(BlockChainVisitor* vis){
        if(!vis->VisitStart()) return false;
        uint256_t hash = GetHead().GetHash();
        do{
            Block* block;
            if(!(block = GetBlockData(hash))) return false;
            if(!vis->Visit(block)) return false;
            hash = block->GetPreviousHash();
        } while(!hash.IsNull());
        return vis->VisitEnd();
    }

    //@Deprecated
    class BlockChainTransactionMerkleTreeBuilder : public BlockChainVisitor,
                                        public BlockVisitor,
                                        public MerkleTreeBuilder{
    private:
        std::vector<uint256_t> leaves_;
    public:
        BlockChainTransactionMerkleTreeBuilder():
            leaves_(){}
        ~BlockChainTransactionMerkleTreeBuilder(){}

        bool Visit(const Block& block){
            return block.Accept(this);
        }

        bool Visit(const Transaction& tx){
            leaves_.push_back(tx.GetHash());
            return true;
        }

        bool BuildTree(){
            if(HasTree()) return false;
            if(!BlockChain::Accept(this)) return false;
            return CreateTree();
        }
    };

    /*
     *TODO:
     * bool Block::Contains(const uint256_t& hash) const{
        BlockMerkleTreeBuilder builder(this);
        if(!builder.BuildTree()) return false;
        MerkleTree* tree = builder.GetTree();
        std::vector<MerkleProofHash> trail;
        if(!tree->BuildAuditProof(hash, trail)) return false;
        return tree->VerifyAuditProof(tree->GetMerkleRootHash(), hash, trail);
    }

    MerkleTree* Block::GetMerkleTree() const{
        BlockMerkleTreeBuilder builder(this);
        if(!builder.BuildTree()) return nullptr;
        return builder.GetTreeCopy();
    }

    uint256_t Block::GetMerkleRoot() const{
        BlockMerkleTreeBuilder builder(this);
        if(!builder.BuildTree()) return uint256_t();
        MerkleTree* tree = builder.GetTree();
        return tree->GetMerkleRootHash();
    }

    MerkleTree* BlockChain::GetMerkleTree(){
        BlockChainMerkleTreeBuilder builder;
        if(!builder.BuildTree()) return nullptr;
        return builder.GetTree();
    }

    bool BlockChain::ContainsTransaction(const uint256_t& hash){
        BlockChainMerkleTreeBuilder builder;
    }

  */

    class BlockChainMerkleTreeBuilder : public MerkleTreeBuilder, BlockChainVisitor{
    public:
        BlockChainMerkleTreeBuilder():
            BlockChainVisitor(),
            MerkleTreeBuilder(){}
        ~BlockChainMerkleTreeBuilder(){}

        bool Visit(Block* block){
            return AddLeaf(block->GetHash());
        }

        bool BuildTree(){
            if(HasTree()) return false;
            if(!BlockChain::Accept(this)) return false;
            return CreateTree();
        }
    };

    MerkleTree* BlockChain::GetMerkleTree(){
         BlockChainMerkleTreeBuilder builder;
         if(!builder.BuildTree()) return nullptr;
         return builder.GetTreeCopy();
    }

    class BlockChainTransactionResolver : public BlockChainVisitor{
    private:
        uint256_t tx_hash_;
        Transaction* result_;

        void SetResult(Transaction* tx){
            result_ = tx;
        }
    public:
        BlockChainTransactionResolver(const uint256_t& tx_hash):
            tx_hash_(tx_hash),
            result_(nullptr){}
        ~BlockChainTransactionResolver(){}

        uint256_t GetTransactionHash() const{
            return tx_hash_;
        }

        Transaction* GetResult() const{
            return result_;
        }

        bool HasResult() const{
            return result_ != nullptr;
        }

        bool Visit(Block* block){
            uint256_t hash = GetTransactionHash();
            if(!block->Contains(hash)) return false;
            SetResult(block->GetTransaction(hash));
            return true;
        }
    };

    Transaction* BlockChain::GetTransaction(const uint256_t& hash){
        BlockChainTransactionResolver resolver(hash);
        BlockChain::Accept(&resolver);
        if(!resolver.HasResult()) return nullptr;
        return resolver.GetResult();
    }
}