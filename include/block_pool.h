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

        static const size_t kMaxPoolSize = 128;
    private:
        BlockPool() = delete;

        static void SetState(State state);
    public:
        ~BlockPool() = delete;

        static State GetState();
        static void Initialize();
        static void RemoveBlock(const uint256_t& hash);
        static void PutBlock(const Handle<Block>& block);
        static bool HasBlock(const uint256_t& hash);
        static bool Accept(BlockPoolVisitor* vis);
        static bool GetBlocks(std::vector<uint256_t>& blocks);
        static Handle<Block> GetBlock(const uint256_t& hash);

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

#ifdef TOKEN_DEBUG
        static void PrintBlocks();
#endif//TOKEN_DEBUG
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