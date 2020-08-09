#include "crash_report.h"
#include "block_pool.h"
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
        if(!BlockPoolIndex::HasData(hash)) return Handle<Block>(); //null
        return BlockPoolIndex::GetData(hash);
    }

    void BlockPool::RemoveBlock(const uint256_t& hash){
        LOCK_GUARD;
        if(!HasBlock(hash)) return;

    }

    void BlockPool::PutBlock(const Handle<Block>& block){
        LOCK_GUARD;
        BlockPoolIndex::PutData(block);
    }

    bool BlockPool::GetBlocks(std::vector<uint256_t>& blocks){
        LOCK_GUARD;
        DIR* dir;
        struct dirent* ent;
        if((dir = opendir(GetDataDirectory().c_str())) != NULL){
            while((ent = readdir(dir)) != NULL){
                std::string name(ent->d_name);
                std::string filename = (GetDataDirectory() + "/" + name);
                if(!EndsWith(filename, ".dat")) continue;

                Handle<Block> block = Block::NewInstance(filename);
                blocks.push_back(block->GetHash());
            }
            closedir(dir);
            return true;
        }
        return false;
    }

    bool BlockPool::Accept(BlockPoolVisitor* vis){
        if(!vis->VisitStart()) return false;

        LOCK_GUARD;
        DIR* dir;
        struct dirent* ent;
        if((dir = opendir(GetDataDirectory().c_str())) != NULL){
            while((ent = readdir(dir)) != NULL){
                std::string name(ent->d_name);
                std::string filename = (GetDataDirectory() + "/" + name);
                if(!EndsWith(filename, ".dat")) continue;

                Handle<Block> block = Block::NewInstance(filename);
                if(!vis->Visit(block)) break;
            }
            closedir(dir);
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