#include <pthread.h>
#include <sstream>
#include <glog/logging.h>
#include "common.h"
#include "block_chain.h"
#include "block_miner.h"
#include "block_validator.h"
#include "object.h"

namespace Token{
#define READ_LOCK pthread_rwlock_tryrdlock(&chain->rwlock_);
#define WRITE_LOCK pthread_rwlock_trywrlock(&chain->rwlock_);
#define UNLOCK pthread_rwlock_unlock(&chain->rwlock_);

    BlockChain::BlockChain():
        IndexManagedPool(FLAGS_path + "/data"),
        rwlock_(),
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

    static bool
    CreateGenesis(Block* genesis){
        Transaction coinbase(0, 0);
        for(uint32_t idx = 0; idx < kNumberOfGenesisOutputs; idx++){
            coinbase << Output("TestUser", "TestToken");
        }
        (*genesis) = Block(0, uint256_t(), { coinbase }, 0);
        return true;
    }

    bool BlockChain::Initialize(){
        BlockChain* chain = GetInstance();
        if(!FileExists(chain->GetRoot())){
            if(!CreateDirectory(chain->GetRoot())) return false;
        }
        if(!chain->InitializeIndex()) return false;;
        if(!chain->HasHeadInIndex()){
            Block genesis;
            if(!CreateGenesis(&genesis)){
                LOG(ERROR) << "couldn't create genesis";
                return false;
            }

            uint256_t hash = genesis.GetHash();
            if(!chain->SaveObject(hash, &genesis)) return false;
            BlockNode* node = new BlockNode(genesis);
            chain->head_ = chain->genesis_ = node;
            chain->nodes_.insert(std::make_pair(hash, node));

            for(auto it : genesis){
                uint32_t index = 0;
                for(auto out = it.outputs_begin(); out != it.outputs_end(); out++){
                    UnclaimedTransaction utxo(it, index++);
                    if(!UnclaimedTransactionPool::PutUnclaimedTransaction(&utxo)){
                        LOG(WARNING) << "couldn't create new unclaimed transaction: " << utxo.GetHash();
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
        BlockNode* node = new BlockNode(block);
        BlockNode* head = node;
        BlockNode* parent = node;
        do{
            LOG(INFO) << "loading block: " << hash << "...";
            if(!(block = GetBlockData(hash))){
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

    BlockChain::BlockNode* BlockChain::GetHeadNode(){
        BlockChain* chain = GetInstance();
        BlockNode* node = chain->head_;
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
        if(pos == chain->nodes_.end()){
            return nullptr;
        }
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
        BlockNode* node = new BlockNode((*block)); // TODO be mindful of de-referencing this pointer?
        parent->AddChild(node);
        BlockNode* head = GetHeadNode();
        if(head->GetHeight() < block->GetHeight()) {
            chain->SetHeadInIndex(hash);
            head_ = node;
        }
        nodes_.insert(std::make_pair(hash, node));

        for(uint32_t txid = 0; txid < block->GetNumberOfTransactions(); txid++){
            Transaction tx;
            if(!block->GetTransaction(txid, &tx)){
                LOG(WARNING) << "couldn't get transaction #" << txid << " from block: " << hash;
                UNLOCK;
                return false;
            }

            uint256_t tx_hash = tx.GetHash();
            uint32_t idx = 0;
            for(auto it = tx.outputs_begin(); it != tx.outputs_end(); it++){
                UnclaimedTransaction utxo(tx_hash, idx++);
                if(!UnclaimedTransactionPool::PutUnclaimedTransaction(&utxo)){
                    LOG(WARNING) << "couldn't create unclaimed transaction for: " << tx_hash << "[" << (idx - 1) << "]";
                    continue;
                }
            }

            for(auto it = tx.inputs_begin(); it != tx.inputs_end(); it++){
                Transaction in_tx;
                if(!BlockChain::GetTransaction(it->GetTransactionHash(), &in_tx)){
                    LOG(ERROR) << "couldn't get transaction: " << it->GetTransactionHash();
                    continue;
                }

                Output in_out;
                if(!in_tx.GetOutput(it->GetOutputIndex(), &in_out)){
                    LOG(ERROR) << "couldn't get output #" << it->GetOutputIndex() << " from transaction: " << it->GetTransactionHash();
                    continue;
                }

                UnclaimedTransaction utxo(in_tx, it->GetOutputIndex());
                uint256_t uhash = utxo.GetHash();
                if(!UnclaimedTransactionPool::RemoveUnclaimedTransaction(uhash)){
                    LOG(ERROR) << "couldn't remove unclaimed transaction: " << uhash;
                    continue;
                }
            }

            if(!TransactionPool::RemoveTransaction(tx.GetHash())){
                LOG(ERROR) << "couldn't remove transaction: " << tx_hash;
                continue;
            }
        }

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

        bool Visit(Block* block){
            uint256_t hash = GetTransactionHash();
            if(!block->Contains(hash)) return true;

            Transaction tx;
            if(!block->GetTransaction(hash, &tx)){
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