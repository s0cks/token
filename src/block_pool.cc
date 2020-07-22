#include "crash_report.h"
#include "block_pool.h"
#include "block_pool_cache.h"
#include "block_pool_index.h"

namespace Token{
    static std::recursive_mutex mutex_;
    static BlockPool::State state_ = BlockPool::kUninitialized;

    static inline std::string
    GetDataDirectory(){
        return FLAGS_path + "/blocks";
    }

#define LOCK_GUARD std::lock_guard<std::recursive_mutex> guard(mutex_);

    void BlockPool::SetState(BlockPool::State state){
        LOCK_GUARD;
        state_ = state;
    }

    BlockPool::State BlockPool::GetState(){
        LOCK_GUARD;
        return state_;
    }

    void BlockPool::Initialize(){
        if(!IsUninitialized()){
            std::stringstream ss;
            ss << "Cannot re-initialize unclaimed transaction pool";
            CrashReport::GenerateAndExit(ss);
        }

        SetState(kInitializing);
        BlockPoolIndex::Initialize();
        SetState(kInitialized);
#ifdef TOKEN_DEBUG
        LOG(INFO) << "initialized the unclaimed transaction pool";
#endif//TOKEN_DEBUG
    }

    bool BlockPool::HasBlock(const uint256_t& hash){
        LOCK_GUARD;
        return BlockPoolIndex::HasData(hash);
    }

    Handle<Block> BlockPool::GetBlock(const uint256_t& hash){
        LOCK_GUARD;
        if(!BlockPoolCache::HasTransaction(hash)){
            if(!BlockPoolIndex::HasData(hash)) return Handle<Block>(); //null
            if(BlockPoolCache::IsFull()) BlockPoolCache::EvictLastUsed();
            Handle<Block> tx = BlockPoolIndex::GetData(hash);
            BlockPoolCache::PutTransaction(hash, tx);
            return tx;
        }
        return BlockPoolCache::GetTransaction(hash);
    }

    void BlockPool::RemoveBlock(const uint256_t& hash){
        LOCK_GUARD;
        //TODO: implement
        if(!HasBlock(hash)) return;
    }

    void BlockPool::PutBlock(const Handle<Block>& utxo){
        LOCK_GUARD;
        uint256_t hash = utxo->GetSHA256Hash();
        if(!BlockPoolCache::HasTransaction(hash)){
            if(BlockPoolCache::IsFull()) BlockPoolCache::EvictLastUsed();
        } else{
            BlockPoolCache::Promote(hash);
        }
        BlockPoolCache::PutTransaction(hash, utxo);
    }

    bool BlockPool::GetBlocks(std::vector<uint256_t>& utxos){
        LOCK_GUARD;
        DIR* dir;
        struct dirent* ent;
        if((dir = opendir(GetDataDirectory().c_str())) != NULL){
            while((ent = readdir(dir)) != NULL){
                std::string name(ent->d_name);
                std::string filename = (GetDataDirectory() + "/" + name);
                if(!EndsWith(filename, ".dat")) continue;
                Block* utxo = Block::NewInstance(filename);
                utxos.push_back(utxo->GetSHA256Hash());
            }
            closedir(dir);
            return true;
        }

        return false;
    }

    bool BlockPool::Accept(BlockPoolVisitor* vis){
        if(!vis->VisitStart()) return false;

        std::vector<uint256_t> utxos;
        if(!GetBlocks(utxos)) return false;

        for(auto& it : utxos){
            Block* utxo;
            if(!(utxo = GetBlock(it))) return false;
            if(!vis->Visit(utxo)) return false;
        }

        return vis->VisitEnd();
    }

#ifdef TOKEN_DEBUG
    class BlockPoolPrinter : public BlockPoolVisitor{
    public:
        BlockPoolPrinter(): BlockPoolVisitor(){}
        ~BlockPoolPrinter() = default;

        bool Visit(const Handle<Block>& blk){
            //TODO: implement
            return true;
        }
    };

    void BlockPool::PrintBlocks(){
        BlockPoolPrinter printer;
        if(!Accept(&printer)){
            //TODO: handle;
        }
    }
#endif//TOKEN_DEBUG
}