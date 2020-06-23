#include <pthread.h>
#include <sstream>
#include <glog/logging.h>

#include "common.h"
#include "keychain.h"
#include "crash_report.h"
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
        if(!Keychain::Initialize()){
            return CrashReport::GenerateAndExit("Couldn't initialize the keychain");
        }

        if(!TransactionPool::Initialize()){
            return CrashReport::GenerateAndExit("Couldn't initialize the Transaction Pool");
        }

        if(!BlockPool::Initialize()){
            return CrashReport::GenerateAndExit("Couldn't initialize the Block Pool");
        }

        if(!UnclaimedTransactionPool::Initialize()){
            return CrashReport::GenerateAndExit("Couldn't initialize the Unclaimed Transaction Pool");
        }

        BlockChain* chain = GetInstance();
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
                    UnclaimedTransaction* utxo = UnclaimedTransaction::NewInstance(it->GetHash(), index++, (*out)->GetUser());
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

            LOG(INFO) << "loaded block " << block->GetHeader();
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

    BlockChain::BlockNode* BlockChain::GetNode(uint32_t height){
        if(height < 0 || height > GetHeight()){
            LOG(WARNING) << "height too big!";
            return nullptr;
        }
        BlockChain* chain = GetInstance();
        READ_LOCK;
        auto pos = chain->blocks_.find(height);
        if(pos == chain->blocks_.end()){
            UNLOCK;
            return nullptr;
        }

        UNLOCK;
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
            BlockHeader block = BlockChain::GetBlock(hash);
            if(!vis->Visit(block)) return false;
            hash = block.GetPreviousHash();
        } while(!hash.IsNull());
        return vis->VisitEnd();
    }
}