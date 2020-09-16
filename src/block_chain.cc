#include <sstream>
#include <glog/logging.h>

#include "common.h"
#include "crash_report.h"
#include "configuration.h"
#include "block_node.h"
#include "block_chain.h"
#include "block_chain_index.h"
#include "block_chain_initializer.h"
#include "unclaimed_transaction_pool.h"

namespace Token{
    static std::recursive_mutex mutex_;
    static BlockChain::State state_;
    static BlockNode* head_ = nullptr;
    static BlockNode* genesis_ = nullptr;

#define LOCK_GUARD std::lock_guard<std::recursive_mutex> guard(mutex_);

    BlockNode* BlockChain::GetHeadNode(){
        LOCK_GUARD;
        return head_;
    }

    void BlockChain::SetHeadNode(BlockNode* node){
        LOCK_GUARD;
        head_ = node;
        BlockChainIndex::PutReference("<HEAD>", node->GetValue().GetHash());
    }

    BlockNode* BlockChain::GetGenesisNode(){
        LOCK_GUARD;
        return genesis_;
    }

    void BlockChain::SetGenesisNode(BlockNode* node){
        LOCK_GUARD;
        genesis_ = node;
        BlockChainIndex::PutReference("<GENESIS>", node->GetValue().GetHash());
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
        return GetHeadNode()->GetValue().GetData();
    }

    Handle<Block> BlockChain::GetGenesis(){
        LOCK_GUARD;
        return GetGenesisNode()->GetValue().GetData();
    }

    Handle<Block> BlockChain::GetBlock(uint32_t height){
        LOCK_GUARD;
        BlockNode* node = GetGenesisNode();
        while(node != nullptr){
            BlockHeader blk = node->GetValue();
            if(blk.GetHeight() == height)
                return node->GetValue().GetData();
            node = node->GetNext();
        }
        return nullptr;
    }

    Handle<Block> BlockChain::GetBlock(const uint256_t& hash){
        BlockNode* node = GetNode(hash);
        return node ?
                node->GetValue().GetData() :
                nullptr;
    }

    BlockNode* BlockChain::GetNode(const uint256_t& hash){
        LOCK_GUARD;
        BlockNode* node = GetGenesisNode();
        while(node != nullptr){
            BlockHeader blk = node->GetValue();
            if(blk.GetHash() == hash)
                return node;
            node = node->GetNext();
        }
        return nullptr;
    }

    bool BlockChain::HasBlock(const uint256_t& hash){
        LOCK_GUARD;
        return !GetBlock(hash).IsNull();
    }

    bool BlockChain::HasTransaction(const uint256_t& hash){
        LOCK_GUARD;
        LOG(WARNING) << "BlockChain::HasTransaction(const uint256_t&) not implemented";
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

        BlockNode* node = new BlockNode(block);
        BlockNode* pnode = GetNode(phash);
        pnode->SetNext(node);
        node->SetNext(pnode->GetNext());
        node->SetPrevious(pnode);
        if(head->GetHeight() < block->GetHeight())
            SetHeadNode(node);
    }

    void BlockChain::Accept(BlockChainVisitor* vis){
        if(!vis->VisitStart()) return;
        BlockNode* node = GetHeadNode();
        do{
            BlockHeader blk = node->GetValue();
            if(!vis->Visit(blk))
                return;
            node = node->GetPrevious();
        } while(node != nullptr);
        if(!vis->VisitStart()) return;
    }

    void BlockChain::Accept(BlockChainDataVisitor* vis){
        BlockNode* node = GetHeadNode();
        while(node != nullptr){
            BlockHeader blk = node->GetValue();
            if(!vis->Visit(blk.GetData()))
                return;
            node = node->GetPrevious();
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