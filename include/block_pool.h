#ifndef TOKEN_BLOCK_POOL_H
#define TOKEN_BLOCK_POOL_H

#include "block.h"

namespace Token{
    class BlockPoolVisitor;
    class BlockPool{
    public:
        enum State{
            kUninitialized,
            kInitializing,
            kInitialized,
        };
    private:
        BlockPool() = delete;

        static void SetState(State state);
    public:
        ~BlockPool() = delete;

        static size_t GetSize();
        static State GetState();
        static bool Initialize();
        static bool Print();
        static bool Accept(BlockPoolVisitor* vis);
        static bool RemoveBlock(const Hash& hash);
        static bool PutBlock(const Handle<Block>& block);
        static bool HasBlock(const Hash& hash);
        static bool GetBlocks(std::vector<Hash>& blocks);
        static Handle<Block> GetBlock(const Hash& hash);
        static void WaitForBlock(const Hash& hash);

        static inline std::string
        GetPath(){
            return TOKEN_BLOCKCHAIN_HOME + "/blocks";
        }

        static inline bool
        IsUninitialized(){
            return GetState() == kUninitialized;
        }

        static inline bool
        IsInitializing(){
            return GetState() == kInitializing;
        }

        static inline bool
        IsInitialized(){
            return GetState() == kInitialized;
        }
    };

    class BlockPoolVisitor{
    protected:
        BlockPoolVisitor() = default;
    public:
        virtual ~BlockPoolVisitor() = default;
        virtual bool VisitStart(){ return true; }
        virtual bool Visit(const Handle<Block>& block) = 0;
        virtual bool VisitEnd(){ return true; }
    };
}

#endif //TOKEN_BLOCK_POOL_H