#include <pthread.h>
#include <sstream>
#include <glog/logging.h>

#include "common.h"
#include "keychain.h"
#include "crash_report.h"
#include "configuration.h"

#include "block_chain.h"
#include "block_miner.h"
#include "block_node.h"
#include "block_chain_index.h"

namespace Token{
    static std::recursive_mutex mutex_;
    static BlockChain::State state_;
    static BlockNode* head_ = nullptr;
    static BlockNode* genesis_ = nullptr;

#define LOCK_GUARD std::lock_guard<std::recursive_mutex> guard(mutex_);

    static BlockNode* GetGenesisNode(){
        return genesis_;
    }

    static BlockNode* GetHeadNode(){
        return head_;
    }

    static inline BlockNode*
    GetNode(const uint256_t& hash){
        BlockNode* current = GetHeadNode();
        while(current != nullptr){
            if(current->GetHash() == hash) return current;
            current = current->GetPrevious();
        }
        return nullptr;
    }

    static inline BlockNode*
    GetNode(uint32_t height){
        BlockNode* current = GetHeadNode();
        while(current != nullptr){
            if(current->GetHeight() == height) return current;
            current = current->GetPrevious();
        }
        return nullptr;
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

        std::vector<Transaction*> transactions = { tx };
        return Block::NewInstance(0, uint256_t(), transactions, 0);
    }

    void BlockChain::SetState(BlockChain::State state){
        LOCK_GUARD;
        state_ = state;
    }

    BlockChain::State BlockChain::GetState(){
        LOCK_GUARD;
        return state_;
    }

    void BlockChain::Initialize(){
        SetState(State::kInitializing);

        Keychain::Initialize(); //TODO: refactor
        TransactionPool::Initialize();//TODO: refactor

        BlockChainConfiguration::Initialize();
        BlockChainIndex::Initialize();

        BlockPool::Initialize();
        UnclaimedTransactionPool::Initialize();

        LOCK_GUARD;
        if(!BlockChainIndex::HasBlockData()){
            Block* genesis = CreateGenesis();
            head_ = genesis_ = BlockNode::NewInstance(genesis);
            BlockChainIndex::PutBlockData(genesis);
            Allocator::AddRoot(genesis);

            for(auto it : (*genesis)){
                uint32_t index = 0;
                for(auto out = it->outputs_begin(); out != it->outputs_end(); out++){
                    UnclaimedTransaction* utxo = UnclaimedTransaction::NewInstance(it->GetHash(), index++, (*out).GetUser());
                    UnclaimedTransactionPool::PutUnclaimedTransaction(utxo);
                }
            }

            BlockChainIndex::PutReference("<HEAD>", genesis->GetHash());
            SetState(State::kInitialized);
            return;
        }

#ifdef TOKEN_DEBUG
        LOG(INFO) << "loading block chain....";
#endif//TOKEN_DEBUG

        uint256_t hash = BlockChainIndex::GetReference("<HEAD>");

#ifdef TOKEN_DEBUG
        LOG(INFO) << "loading block: " << hash;
#endif//TOKEN_DEBUG

        Block* block = BlockChainIndex::GetBlockData(hash);
        BlockNode* node = BlockNode::NewInstance(block);
        Allocator::AddRoot(node);

        head_ = node;
        while(true){
            hash = block->GetPreviousHash();

#ifdef TOKEN_DEBUG
            LOG(INFO) << "loading block: " << hash;
#endif//TOKEN_DEBUG

            block = BlockChainIndex::GetBlockData(hash);
            BlockNode* current = BlockNode::NewInstance(block);
            Allocator::AddRoot(current);

            node->SetPrevious(current);
            current->SetNext(node);

            if(block->IsGenesis()){
                genesis_ = current;
                break;
            }
            node = current;
        }
    }

    BlockHeader BlockChain::GetHead(){
        LOCK_GUARD;
        return GetHeadNode()->GetBlock();
    }

    BlockHeader BlockChain::GetGenesis(){
        LOCK_GUARD;
        return GetGenesisNode()->GetBlock();
    }

    BlockHeader BlockChain::GetBlock(uint32_t height){
        LOCK_GUARD;
        return GetNode(height)->GetBlock();
    }

    BlockHeader BlockChain::GetBlock(const uint256_t& hash){
        LOCK_GUARD;
        return GetNode(hash)->GetBlock();
    }

    Block* BlockChain::GetBlockData(const uint256_t& hash){
        return BlockChainIndex::GetBlockData(hash);
    }

    bool BlockChain::HasBlock(const uint256_t& hash){
        LOCK_GUARD;
        return GetNode(hash) != nullptr;
    }

    void BlockChain::Append(Block* block){
        LOCK_GUARD;

        BlockHeader head = BlockChain::GetHead();
#ifdef TOKEN_DEBUG
        LOG(INFO) << "<HEAD>:";
        LOG(INFO) << "  hash := " << head.GetHash();
        LOG(INFO) << "  parent hash := " << head.GetPreviousHash();

        LOG(INFO) << "appending new block:";
        LOG(INFO) << "  hash := " << block->GetHash();
        LOG(INFO) << "  parent hash := " << block->GetPreviousHash();
#endif//TOKEN_DEBUG
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

        BlockChainIndex::PutBlockData(block);

        BlockNode* parent = GetNode(phash);
        BlockNode* node = BlockNode::NewInstance(block);
        node->SetNext(parent->GetNext());
        node->SetPrevious(parent);
        parent->SetNext(node);
        if(head.GetHeight() < block->GetHeight()) {
            BlockChainIndex::PutReference("<HEAD>", hash);
            head_ = node;
        }
    }

    void BlockChain::Accept(BlockChainVisitor* vis){
        if(!vis->VisitStart()) return;
        uint256_t hash = GetHead().GetHash();
        do{
            BlockHeader block = BlockChain::GetBlock(hash);
            if(!vis->Visit(block)) return;
            hash = block.GetPreviousHash();
        } while(!hash.IsNull());
    }
}