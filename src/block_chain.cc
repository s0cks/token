#include <sstream>
#include <glog/logging.h>

#include "common.h"
#include "crash_report.h"
#include "configuration.h"
#include "block_chain.h"
#include "node.h"
#include "block_chain_index.h"
#include "block_chain_initializer.h"
#include "unclaimed_transaction_pool.h"

namespace Token{
    static std::recursive_mutex mutex_;
    static BlockChain::State state_;
    static Block* head_ = nullptr;
    static Block* genesis_ = nullptr;

#define LOCK_GUARD std::lock_guard<std::recursive_mutex> guard(mutex_);

    void BlockChain::SetHead(const Handle<Block>& blk){
        LOCK_GUARD;
        head_ = blk;
        BlockChainIndex::PutReference("<HEAD>", blk->GetHash());
    }

    void BlockChain::SetGenesis(const Handle<Block>& blk){
        LOCK_GUARD;
        genesis_ = blk;
    }

    void BlockChain::SetState(BlockChain::State state){
        LOCK_GUARD;
        state_ = state;
    }

    BlockChain::State BlockChain::GetState(){
        LOCK_GUARD;
        return state_;
    }

    static inline bool
    ShouldLoadSnapshot(){
        return !FLAGS_snapshot.empty();
    }

    bool BlockChain::Initialize(){
        if(IsInitialized()){
            LOG(WARNING) << "cannot reinitialize the block chain!";
            return false;
        }

        if(ShouldLoadSnapshot()){
            Snapshot* snapshot = nullptr;
            if(!(snapshot = Snapshot::ReadSnapshot(FLAGS_snapshot))){
                LOG(WARNING) << "couldn't load snapshot: " << FLAGS_snapshot;
                return false;
            }

            SnapshotBlockChainInitializer initializer(snapshot);
            if(!initializer.Initialize()){
                LOG(WARNING) << "couldn't initialize block chain from snapshot: " << FLAGS_snapshot;
                return false;
            }
            return true;
        }

        DefaultBlockChainInitializer initializer;
        if(!initializer.Initialize()){
            LOG(WARNING) << "block chain wasn't initialized";
            return false;
        }
        return true;
    }

    Handle<Block> BlockChain::GetHead(){
        LOCK_GUARD;
        return head_;
    }

    Handle<Block> BlockChain::GetGenesis(){
        LOCK_GUARD;
        return genesis_;
    }

    Handle<Block> BlockChain::GetBlock(uint32_t height){
        LOCK_GUARD;
        Handle<Block> node = GetGenesis();
        while(node != nullptr){
            if(node->GetHeight() == height) return node;
            node = node->GetNext().CastTo<Block>();
        }
        return nullptr;
    }

    Handle<Block> BlockChain::GetBlock(const uint256_t& hash){
        LOCK_GUARD;
        Handle<Block> node = GetGenesis();
        while(node != nullptr){
            if(node->GetHash() == hash) return node;
            node = node->GetNext().CastTo<Block>();
        }
        return nullptr;
    }

    bool BlockChain::HasBlock(const uint256_t& hash){
        LOCK_GUARD;
        return !GetBlock(hash).IsNull();
    }

    bool BlockChain::HasTransaction(const uint256_t& hash){
        LOCK_GUARD;
        Handle<Block> node = GetGenesis();
        while(node != nullptr){
            if(node->Contains(hash)) return true;
            node = node->GetNext().CastTo<Block>();
        }
        return false;
    }

    void BlockChain::Append(const Handle<Block>& block){
        LOCK_GUARD;

        Handle<Block> head = GetHead();
        uint256_t hash = block->GetHash();
        uint256_t phash = block->GetPreviousHash();

#ifdef TOKEN_DEBUG
        LOG(INFO) << "appending new block:";
        LOG(INFO) << "  - Parent Hash: " << phash;
        LOG(INFO) << "  - Hash: " << hash;
#endif//TOKEN_DEBUG

        if(BlockChainIndex::HasBlockData(hash)){
            LOG(WARNING) << "duplicate block found for: " << hash;
            return;
        }

        if(block->IsGenesis()){
            LOG(WARNING) << "cannot append genesis block: " << hash;
            return;
        }

        if(phash != head->GetHash()){
            LOG(WARNING) << "parent hash '" << phash << "' doesn't match <HEAD> hash: " << head->GetHash();
            return;
        }

        BlockChainIndex::PutBlockData(block);

        Handle<Block> parent = GetBlock(phash);
        parent->SetNext(block.CastTo<Node>());
        block->SetNext(parent->GetNext());
        block->SetPrevious(parent.CastTo<Node>());

        if(head->GetHeight() < block->GetHeight())
            SetHead(block);
    }

    void BlockChain::Accept(BlockChainVisitor* vis){
        if(!vis->VisitStart()) return;
        Handle<Block> node = GetHead();
        do{
            if(!vis->Visit(node->GetHeader())) return;
            node = node->GetPrevious().CastTo<Block>();
        }while(node != nullptr);
        if(!vis->VisitStart()) return;
    }

    void BlockChain::Accept(BlockChainDataVisitor* vis){
        Handle<Block> current = GetHead();
        while(current != nullptr){
            if(!vis->Visit(current)) return;
            current = current->GetPrevious().CastTo<Block>();
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