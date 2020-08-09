#include <sstream>
#include <glog/logging.h>

#include "common.h"
#include "keychain.h"
#include "crash_report.h"
#include "configuration.h"

#include "block_chain.h"
#include "block_node.h"
#include "block_chain_index.h"
#include "block_pool.h"
#include "transaction_pool.h"
#include "unclaimed_transaction_pool.h"

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

        if(!FileExists(TOKEN_BLOCKCHAIN_HOME)){
            if(!CreateDirectory(TOKEN_BLOCKCHAIN_HOME)){
                std::stringstream ss;
                ss << "Couldn't initialize block chain in directory: " << TOKEN_BLOCKCHAIN_HOME;
                CrashReport::GenerateAndExit(ss);
            }
        }

        Keychain::Initialize();
        BlockChainConfiguration::Initialize();
        BlockChainIndex::Initialize();

        BlockPool::Initialize();
        TransactionPool::Initialize();
        UnclaimedTransactionPool::Initialize();

        LOCK_GUARD;
        if(!BlockChainIndex::HasBlockData()){
            Handle<Block> genesis = Block::Genesis();
            head_ = genesis_ = BlockNode::NewInstance(genesis);
            BlockChainIndex::PutBlockData(genesis);

            for(uint32_t idx = 0; idx < genesis->GetNumberOfTransactions(); idx++){
                Transaction* it = genesis->GetTransaction(idx);

                for(uint32_t out_idx = 0; out_idx < it->GetNumberOfOutputs(); out_idx++){
                    Handle<Output> out_it = it->GetOutput(out_idx);
                    Handle<UnclaimedTransaction> out_utxo = UnclaimedTransaction::NewInstance(it->GetHash(), out_idx, out_it->GetUser());
                    UnclaimedTransactionPool::PutUnclaimedTransaction(out_utxo);
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
        Block* block = BlockChainIndex::GetBlockData(hash);
#ifdef TOKEN_DEBUG
        LOG(INFO) << "loading block: " << block->GetHeader();
#endif//TOKEN_DEBUG

        BlockNode* node = BlockNode::NewInstance(block);

        head_ = node;
        while(true){
            hash = block->GetPreviousHash();
            if(hash.IsNull()){
                genesis_ = node;
                break;
            }

            block = BlockChainIndex::GetBlockData(hash);
#ifdef TOKEN_DEBUG
            LOG(INFO) << "loading block: " << block->GetHeader();
#endif//TOKEN_DEBUG
            BlockNode* current = BlockNode::NewInstance(block);

            node->SetPrevious(current);
            current->SetNext(node);

            if(block->IsGenesis()){
                genesis_ = current;
                break;
            }
            node = current;
        }
        SetState(kInitialized);
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

    Handle<Block> BlockChain::GetBlockData(const uint256_t& hash){
        return BlockChainIndex::GetBlockData(hash);
    }

    bool BlockChain::HasBlock(const uint256_t& hash){
        LOCK_GUARD;
        return GetNode(hash) != nullptr;
    }

    void BlockChain::Append(Block* block){
        LOCK_GUARD;

        BlockHeader head = BlockChain::GetHead();
        uint256_t hash = block->GetHash();
        uint256_t phash = block->GetPreviousHash();

#ifdef TOKEN_DEBUG
        LOG(INFO) << "appending new block:";
        LOG(INFO) << "  - Parent Hash: " << phash;
        LOG(INFO) << "  - Hash: " << hash;
#endif//TOKEN_DEBUG

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
        BlockNode* node = GetHeadNode();
        do{
            if(!vis->Visit(node->GetBlock())) return;
            node = node->GetPrevious();
        }while(node != nullptr);
        if(!vis->VisitStart()) return;
    }

    void BlockChain::Accept(BlockChainDataVisitor* vis){
        BlockNode* node = GetGenesisNode();
        while(node != nullptr){
            Handle<Block> blk = node->GetData();
            if(!vis->Visit(blk)) return;
            node = node->GetNext();
        }
    }

#ifdef TOKEN_DEBUG
    class BlockChainPrinter : public BlockChainVisitor{
    public:
        BlockChainPrinter(): BlockChainVisitor(){}

        bool Visit(const BlockHeader& block){
            LOG(INFO) << " - " << block;
            return true;
        }
    };

    void BlockChain::PrintBlockChain(){
        BlockChainPrinter printer;
        Accept(&printer);
    }
#endif//TOKEN_DEBUG
}