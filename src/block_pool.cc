#include "block_pool.h"
#include "journal.h"
#include "crash_report.h"

namespace Token{
    static std::recursive_mutex mutex_;
    static std::condition_variable_any cond_;
    static BlockPool::State state_ = BlockPool::kUninitialized;

#define LOCK_GUARD std::lock_guard<std::recursive_mutex> guard(mutex_);
#define LOCK std::unique_lock<std::recursive_mutex> lock(mutex_)
#define UNLOCK lock.unlock()
#define WAIT cond_.wait(lock)
#define SIGNAL_ONE cond_.notify_one()
#define SIGNAL_ALL cond_.notify_all()

    static inline ObjectPoolJournal<Block>*
    GetJournal(){
        static ObjectPoolJournal<Block> journal(BlockPool::GetPath());
        return &journal;
    }

    void BlockPool::SetState(BlockPool::State state){
        LOCK_GUARD;
        state_ = state;
    }

    BlockPool::State BlockPool::GetState(){
        LOCK_GUARD;
        return state_;
    }

    bool BlockPool::Initialize(){
        if(IsInitialized()){
            LOG(WARNING) << "cannot re-initialize the block pool";
            return false;
        }

        SetState(BlockPool::kInitializing);
        if(!GetJournal()->IsInitialized()){
            LOG(WARNING) << "couldn't initialize the block pool journal in path: " << GetPath();
            SetState(BlockPool::kUninitialized);
            return false;
        }

        SetState(BlockPool::kInitialized);
        LOG(INFO) << "initialized the unclaimed transaction pool";
        return true;
    }

    bool BlockPool::HasBlock(const Hash& hash){
        LOCK_GUARD;
        return GetJournal()->HasData(hash);
    }

    Handle<Block> BlockPool::GetBlock(const Hash& hash){
        LOCK_GUARD;
        return GetJournal()->HasData(hash) ?
                GetJournal()->GetData(hash) :
                nullptr;
    }

    bool BlockPool::RemoveBlock(const Hash& hash){
        LOCK_GUARD;
        return GetJournal()->HasData(hash) ?
                GetJournal()->RemoveData(hash) :
                false;
    }

    bool BlockPool::PutBlock(const Handle<Block>& block){
        LOCK_GUARD;
        Hash hash = block->GetHash();
        if(GetJournal()->HasData(hash))
            return false;
        if(!GetJournal()->PutData(block)){
            LOG(WARNING) << "couldn't put block " << hash << " in pool.";
            return false;
        }
        SIGNAL_ALL;
        return true;
    }

    size_t BlockPool::GetSize(){
        LOCK_GUARD;
        size_t size = 0;
        DIR* dir;
        struct dirent* ent;
        if((dir = opendir(GetPath().c_str())) != NULL){
            while((ent = readdir(dir)) != NULL){
                std::string name(ent->d_name);
                std::string filename = (GetPath() + "/" + name);
                if(!EndsWith(filename, ".dat")) continue;
                size++;
            }
            closedir(dir);
        }
        return size;
    }

    bool BlockPool::GetBlocks(std::vector<Hash>& blocks){
        LOCK_GUARD;
        DIR* dir;
        struct dirent* ent;
        if((dir = opendir(GetPath().c_str())) != NULL){
            while((ent = readdir(dir)) != NULL){
                std::string name(ent->d_name);
                std::string filename = (GetPath() + "/" + name);
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
        if((dir = opendir(GetPath().c_str())) != NULL){
            while((ent = readdir(dir)) != NULL){
                std::string name(ent->d_name);
                std::string filename = (GetPath() + "/" + name);
                if(!EndsWith(filename, ".dat")) continue;

                Handle<Block> block = Block::NewInstance(filename);
                if(!vis->Visit(block)) break;
            }
            closedir(dir);
        }

        return vis->VisitEnd();
    }

    void BlockPool::WaitForBlock(const Hash& hash){
        LOCK;
        while(!HasBlock(hash)) WAIT;
    }

    class BlockPoolPrinter : public BlockPoolVisitor, public ObjectPointerVisitor{
    public:
        BlockPoolPrinter(): BlockPoolVisitor(){}
        ~BlockPoolPrinter() = default;

        bool Visit(Object* obj){
            //TODO: implement BlockPoolPrinter::Visit(RawObject*)
            return true;
        }

        bool Visit(const Handle<Block>& blk){
            return Visit((Object*)blk);
        }
    };

    bool BlockPool::Print(){
        BlockPoolPrinter printer;
        return Accept(&printer);
    }
}