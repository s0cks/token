#include <sstream>
#include <glog/logging.h>
#include "common.h"
#include "block_chain.h"
#include "block_miner.h"
#include "block_validator.h"

namespace Token{
#define READ_LOCK pthread_rwlock_tryrdlock(&chain->rwlock_);
#define WRITE_LOCK pthread_rwlock_trywrlock(&chain->rwlock_);
#define UNLOCK pthread_rwlock_unlock(&chain->rwlock_);

    BlockChain::BlockChain():
        IndexManagedPool(FLAGS_path + "/blocks"),
        rwlock_(PTHREAD_RWLOCK_INITIALIZER),
        genesis_(nullptr),
        head_(nullptr){}

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

    static Block*
    CreateGenesis(){
        Transaction coinbase(0, 0);
        coinbase << Output("TestUser", "TestToken");
        return new Block(0, uint256_t(), { coinbase }, 0);
    }

    bool BlockChain::Initialize(){
        BlockChain* chain = GetInstance();
        if(!FileExists(chain->GetRoot())){
            if(!CreateDirectory(chain->GetRoot())) return false;
        }
        if(!chain->InitializeIndex()) return false;;
        if(!chain->HasHeadInIndex()){
            Transaction coinbase(0, 0);
            coinbase << Output("TestUser", "TestToken");
            Block genesis(0, uint256_t(), { coinbase }, 0);
            uint256_t hash = genesis.GetHash();
            std::string filename = chain->GetPath(hash);
            if(!chain->SaveBlock(filename, &genesis)) return false;
            if(!chain->SetHeadInIndex(hash)) return false;
            if(!chain->RegisterPath(hash, filename)) return false;
            BlockNode* node = new BlockNode(genesis);
            chain->head_ = chain->genesis_ = node;
            chain->nodes_.insert(std::make_pair(hash, node));

            LOG(INFO) << "processing " << genesis.GetNumberOfTransactions() << " transactions....";

            int i;
            for(i = 0; i < genesis.GetNumberOfTransactions(); i++){
                Transaction tx;
                if(!genesis.GetTransaction(i, &tx)){
                    LOG(ERROR) << "couldn't get transaction #" << i << " in block: " << hash;
                    UNLOCK;
                    return false;
                }

                LOG(INFO) << "processing " << tx.GetHash() << " w/ " << tx.GetNumberOfInputs() << " inputs + " << tx.GetNumberOfOutputs() << " outputs";

                int j;
                for(j = 0; j < tx.GetNumberOfOutputs(); j++) {
                    Output out;
                    if(!tx.GetOutput(j, &out)){
                        LOG(WARNING) << "couldn't get output #" << j;
                        UNLOCK;
                        return false;
                    }

                    UnclaimedTransaction utxo(tx, out);
                    if(!UnclaimedTransactionPool::PutUnclaimedTransaction(&utxo)){
                        LOG(WARNING) << "couldn't create new unclaimed transaction: " << utxo.GetHash();
                        LOG(WARNING) << "*** Unclaimed Transaction: ";
                        LOG(WARNING) << "***   + Input: " << utxo.GetTransaction() << "[" << utxo.GetIndex()
                                     << "]";
                        LOG(WARNING) << "***   + Output: " << utxo.GetToken() << "(" << utxo.GetUser() << ")";
                        UNLOCK;
                        return false;
                    }

                    LOG(INFO) << "created new unclaimed transaction: " << utxo.GetHash() << "(" << utxo.GetTransaction() << "[" << utxo.GetIndex() << "])";
                }
            }
            return true;
        }

        uint256_t hash = chain->GetHeadFromIndex();
        Block block;
        if(!GetBlockData(hash, &block)) return false;
        BlockNode* node = new BlockNode(block);
        BlockNode* head = node;
        BlockNode* parent = node;
        do{
            LOG(INFO) << "loading block: " << hash << "...";
            if(!GetBlockData(hash, &block)){
                LOG(ERROR) << "cannot load block data!";
                return false;
            }
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
        READ_LOCK;
        BlockNode* node = chain->head_;
        UNLOCK;
        return node;
    }

    BlockChain::BlockNode* BlockChain::GetGenesisNode(){
        BlockChain* chain = GetInstance();
        READ_LOCK;
        BlockNode* node = chain->genesis_;
        UNLOCK;
        return node;
    }

    BlockChain::BlockNode* BlockChain::GetNode(const uint256_t& hash){
        BlockChain* chain = GetInstance();
        READ_LOCK;
        auto pos = chain->nodes_.find(hash);
        if(pos == chain->nodes_.end()){
            UNLOCK;
            return nullptr;
        }
        UNLOCK;
        return pos->second;
    }

    bool BlockChain::GetBlockData(const uint256_t& hash, Block* result){
        BlockChain* chain = GetInstance();
        //TODO: rwlock
        std::string filename = chain->GetPath(hash);
        if(!FileExists(filename)){
            LOG(INFO) << "no block found for: " << filename;
            return false;
        }
        return chain->LoadBlock(filename, result);
    }

    bool BlockChain::Append(Block* block){
        WRITE_LOCK;
        BlockChain* chain = GetInstance();
        uint256_t hash = block->GetHash();
        uint256_t phash = block->GetPreviousHash();

        std::string filename = chain->GetPath(hash);
        if(FileExists(filename)){
            LOG(WARNING) << "block data already exists";
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

        if(!BlockValidator::IsValid(block)){
            LOG(ERROR) << "the following block contains invalid transactions: " << hash;
            UNLOCK;
            return false;
        }

        if(!SaveBlock(filename, block)) {
            LOG(INFO) << "couldn't save block: " << filename;
            UNLOCK;
            return false;
        }
        if(!chain->RegisterPath(hash, filename)) {
            LOG(INFO) << "couldn't register path: " << hash;
            UNLOCK;
            return false;
        }

        LOG(INFO) << "phash := " << phash;
        BlockNode* parent = GetNode(phash);
        BlockNode* node = new BlockNode((*block)); // TODO be mindful of de-referencing this pointer?
        parent->AddChild(node);
        BlockNode* head = GetHeadNode();
        if(head->GetHeight() < block->GetHeight()) {
            chain->SetHeadInIndex(hash);
            head_ = node;
        }
        nodes_.insert(std::make_pair(hash, node));
        UNLOCK;
        LOG(INFO) << "appended block: " << hash;
        return true;
    }

    bool BlockChain::Accept(BlockChainVisitor* vis){
        if(!vis->VisitStart()) return false;
        uint256_t hash = GetHead().GetHash();
        do{
            Block block;
            if(!GetBlockData(hash, &block)) return false;
            if(!vis->Visit(block)) return false;
            hash = block.GetPreviousHash();
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

        bool Visit(const Block& block){
            return AddLeaf(block.GetHash());
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
            result_(nullptr){
        }
        ~BlockChainTransactionResolver(){
            delete result_;
        }

        uint256_t GetTransactionHash() const{
            return tx_hash_;
        }

        Transaction* GetResult() const{
            return result_;
        }

        bool HasResult() const{
            return result_ != nullptr;
        }

        bool Visit(const Block& block){
            uint256_t hash = GetTransactionHash();
            if(!block.Contains(hash)) return true;

            Transaction tx;
            if(!block.GetTransaction(hash, &tx)){
                LOG(WARNING) << "unable to get transaction: " << hash;
                SetResult(nullptr);
                return false;
            }

            SetResult(new Transaction(tx));
            return false;
        }
    };

    bool BlockChain::GetTransaction(const uint256_t& hash, Transaction* result){
        BlockChainTransactionResolver resolver(hash);
        BlockChain::Accept(&resolver);
        if(!resolver.HasResult()) return false;
        (*result) = (*resolver.GetResult());
        return true;
    }
}