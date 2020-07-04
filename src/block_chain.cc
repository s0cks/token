#include <pthread.h>
#include <sstream>
#include <glog/logging.h>

#include "common.h"
#include "keychain.h"
#include "crash_report.h"
#include "block_chain.h"
#include "block_chain_index.h"
#include "block_miner.h"
#include "object.h"

namespace Token{
    static pthread_mutex_t mutex_ = PTHREAD_MUTEX_INITIALIZER;

    BlockChain::BlockChain():
        nodes_(),
        blocks_(),
        genesis_(nullptr),
        head_(nullptr){
    }

    BlockChain*
    BlockChain::GetInstance(){
        static BlockChain instance;
        return &instance;
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
            cb_outputs.push_back(Output("TestUser", token.str()));
        }

        Transaction* tx = Transaction::NewInstance(0, cb_inputs, cb_outputs, 0);
        Allocator::AddRoot(tx);

        std::vector<Transaction*> transactions = { tx };
        Block* blk = Block::NewInstance(0, uint256_t(), transactions, 0);

        Allocator::RemoveRoot(tx);
        return blk;
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

        UnclaimedTransactionPool::Initialize();
        BlockChainIndex::Initialize();

        BlockChain* chain = GetInstance();
        if(!BlockChainIndex::HasBlockData()){
            Block* genesis;
            if(!(genesis = CreateGenesis())) CrashReport::GenerateAndExit("Couldn't generate genesis block");
            Allocator::AddRoot(genesis);

            uint256_t hash = genesis->GetHash();
            BlockChainIndex::PutBlockData(genesis);
            BlockNode* node = new BlockNode(genesis);
            chain->head_ = chain->genesis_ = node;
            chain->blocks_.insert(std::make_pair(genesis->GetHeight(), node));
            chain->nodes_.insert(std::make_pair(hash, node));

            for(auto it : (*genesis)){
                uint32_t index = 0;
                for(auto out = it->outputs_begin(); out != it->outputs_end(); out++){
                    UnclaimedTransaction* utxo = UnclaimedTransaction::NewInstance(it->GetHash(), index++, (*out).GetUser());
                    Allocator::AddRoot(utxo);
                    UnclaimedTransactionPool::PutUnclaimedTransaction(utxo);
                    Allocator::RemoveRoot(utxo);
                }
            }

            BlockChainIndex::PutReference("<HEAD>", hash);
            return true;
        }

        uint256_t hash = BlockChainIndex::GetReference("<HEAD>");
        Block* block = nullptr;
        if(!(block = GetBlockData(hash))){
            std::stringstream ss;
            ss << "Couldn't load the <HEAD> block: " << hash;
            CrashReport::GenerateAndExit(ss.str());
        }

        Allocator::AddRoot(block);

        BlockNode* node = new BlockNode(block);
        BlockNode* head = node;
        BlockNode* parent = node;
        do{
            if(!(block = GetBlockData(hash))){
                std::stringstream ss;
                ss << "Couldn't load block: " << hash;
                CrashReport::GenerateAndExit(ss.str());
            }

            Allocator::AddRoot(block);

#if defined(TOKEN_ENABLE_DEBUG)
            LOG(INFO) << "loaded block: " << block->GetHeader();
#endif//TOKEN_ENABLE_DEBUG

            node = new BlockNode(block);
            chain->blocks_.insert({ node->GetHeight(), node });
            chain->nodes_.insert({ hash, node });
            parent->AddChild(node);
            parent = node;
            hash = node->GetPreviousHash();
        } while(!hash.IsNull());

        chain->head_ = head;
        chain->nodes_.insert({ head->GetHash(), head });
        chain->blocks_.insert({ head->GetHeight(), head });
        chain->genesis_ = node;
        return true;
    }

    BlockChain::BlockNode* BlockChain::GetHeadNode(){
        BlockChain* chain = GetInstance();
        pthread_mutex_trylock(&mutex_);
        BlockNode* node = chain->head_;
        pthread_mutex_unlock(&mutex_);
        return node;
    }

    BlockChain::BlockNode* BlockChain::GetGenesisNode(){
        BlockChain* chain = GetInstance();
        pthread_mutex_trylock(&mutex_);
        BlockNode* node = chain->genesis_;
        pthread_mutex_unlock(&mutex_);
        return node;
    }

    BlockChain::BlockNode* BlockChain::GetNode(const uint256_t& hash){
        BlockChain* chain = GetInstance();

        pthread_mutex_trylock(&mutex_);
        auto pos = chain->nodes_.find(hash);
        if(pos == chain->nodes_.end()){
            pthread_mutex_unlock(&mutex_);
            return nullptr;
        }

        pthread_mutex_unlock(&mutex_);
        return pos->second;
    }

    BlockChain::BlockNode* BlockChain::GetNode(uint32_t height){
        if(height < 0 || height > GetHeight()) return nullptr;

        BlockChain* chain = GetInstance();
        pthread_mutex_trylock(&mutex_);
        auto pos = chain->blocks_.find(height);
        if(pos == chain->blocks_.end()){
            pthread_mutex_unlock(&mutex_);
            return nullptr;
        }

        pthread_mutex_unlock(&mutex_);
        return pos->second;
    }

    Block* BlockChain::GetBlockData(const uint256_t& hash){
        BlockChain* chain = GetInstance();
        pthread_mutex_trylock(&mutex_);
        Block* block = BlockChainIndex::GetBlockData(hash);
        pthread_mutex_unlock(&mutex_);
        return block;
    }

    bool BlockChain::Append(Block* block){
        BlockChain* chain = GetInstance();

#if defined(TOKEN_ENABLE_DEBUG)
        BlockHeader head = GetHead();
        LOG(INFO) << "<HEAD>:";
        LOG(INFO) << "  hash := " << head.GetHash();
        LOG(INFO) << "  parent hash := " << head.GetPreviousHash();

        LOG(INFO) << "appending new block:";
        LOG(INFO) << "  hash := " << block->GetHash();
        LOG(INFO) << "  parent hash := " << block->GetPreviousHash();
#endif//TOKEN_ENABLE_DEBUG

        pthread_mutex_trylock(&mutex_);
        uint256_t hash = block->GetHash();
        uint256_t phash = block->GetPreviousHash();

        if(BlockChainIndex::HasBlockData(hash)){
            std::stringstream ss;
            ss << "Duplicate block found for: " << hash;
            CrashReport::GenerateAndExit(ss);
        }

        if(block->IsGenesis()){
            std::stringstream ss;
            ss << "Cannot append genesis block: " << hash;
            CrashReport::GenerateAndExit(ss.str());
        }

        if(phash != head.GetHash()){
            std::stringstream ss;
            ss << "Parent hash '" << phash << "' doesn't match <HEAD> hash: " << head.GetHash();
            CrashReport::GenerateAndExit(ss.str());
        }

        if(!chain->ContainsBlock(phash)){
            LOG(WARNING) << "couldn't find parent: " << phash;
            pthread_mutex_unlock(&mutex_);
            return false;
        }

        BlockChainIndex::PutBlockData(block);

        BlockNode* parent = GetNode(phash);
        BlockNode* node = new BlockNode(block);
        parent->AddChild(node);
        if(head.GetHeight() < block->GetHeight()) {
            BlockChainIndex::PutReference("<HEAD>", hash);
            head_ = node;
        }

        blocks_.insert({ node->GetHeight(), node });
        nodes_.insert({ hash, node });
        pthread_mutex_unlock(&mutex_);
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