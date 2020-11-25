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

    BlockHeader BlockChain::GetHead(){
        LOCK_GUARD;
        return GetHeadNode()->GetValue();
    }

    BlockHeader BlockChain::GetGenesis(){
        LOCK_GUARD;
        return GetGenesisNode()->GetValue();
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

    bool BlockChain::GetHeaders(std::set<BlockHeader>& blocks){
        LOCK_GUARD;
        BlockNode* node = GetGenesisNode();
        while(node != nullptr){
            BlockHeader blk = node->GetValue();
            blocks.insert(blk);
            node = node->GetNext();
        }
        return !blocks.empty();
    }

    Handle<Block> BlockChain::GetBlock(const Hash& hash){
        if(!BlockChainIndex::HasBlockData(hash)) return nullptr;
        return BlockChainIndex::GetBlockData(hash);
    }

    BlockNode* BlockChain::GetNode(const Hash& hash){
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

    bool BlockChain::HasBlock(const Hash& hash){
        LOCK_GUARD;
        return BlockChainIndex::HasBlockData(hash);
    }

    bool BlockChain::HasTransaction(const Hash& hash){
        LOCK_GUARD;
        LOG(WARNING) << "BlockChain::HasTransaction(const Hash&) not implemented";
        return false;
    }

    void BlockChain::Append(const Handle<Block>& block){
        LOCK_GUARD;
        BlockHeader head = GetHead();
        Hash hash = block->GetHash();
        Hash phash = block->GetPreviousHash();

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

        if(phash != head.GetHash()){
            LOG(WARNING) << "parent Hash '" << phash << "' doesn't match <HEAD> Hash: " << head.GetHash();
            return;
        }

        BlockChainIndex::PutBlockData(block);

        BlockNode* node = new BlockNode(block);
        BlockNode* pnode = GetNode(phash);
        pnode->SetNext(node);
        node->SetNext(pnode->GetNext());
        node->SetPrevious(pnode);
        if(head.GetHeight() < block->GetHeight())
            SetHeadNode(node);
    }

    bool BlockChain::Accept(BlockChainVisitor* vis){
        if(!vis->VisitStart()) return false;
        BlockNode* node = GetHeadNode();
        do{
            BlockHeader blk = node->GetValue();
            if(!vis->Visit(blk))
                return false;
            node = node->GetPrevious();
        } while(node != nullptr);
        return vis->VisitEnd();
    }

    bool BlockChain::Accept(BlockChainDataVisitor* vis){
        BlockNode* node = GetHeadNode();
        while(node != nullptr){
            BlockHeader blk = node->GetValue();
            if(!vis->Visit(blk.GetData()))
                return false;
            node = node->GetPrevious();
        }
        return true;
    }

    class DefaultBlockChainPrinter : public BlockChainVisitor{
    public:
        bool Visit(const BlockHeader& block){
            LOG(INFO) << " - " << block;
            return true;
        }
    };

    class DetailedBlockChainPrinter : public BlockChainDataVisitor{
    public:
        bool Visit(const Handle<Block>& blk){
            LOG(INFO) << blk;
            return true;
        }
    };

    bool BlockChain::Print(bool is_detailed){
        if(is_detailed){
            DetailedBlockChainPrinter printer;
            return Accept(&printer);
        } else{
            DefaultBlockChainPrinter printer;
            return Accept(&printer);
        }
    }
}