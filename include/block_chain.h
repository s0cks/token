#ifndef TOKEN_BLOCK_CHAIN_H
#define TOKEN_BLOCK_CHAIN_H

#include <leveldb/db.h>
#include <map>

#include "common.h"
#include "merkle.h"
#include "block.h"
#include "transaction.h"
#include "unclaimed_transaction.h"

namespace Token{
    class Node;
    class BlockChainVisitor;
    class BlockChainDataVisitor;
    class BlockChain{
        friend class Server;
        friend class BlockChainInitializer;
    public:
        enum State{
            kUninitialized,
            kInitializing,
            kInitialized
        };
    private:
        BlockChain() = delete;

        static void SetHead(const Handle<Block>& blk);
        static void SetGenesis(const Handle<Block>& blk);
        static void SetState(State state);
    public:
        ~BlockChain() = delete;

        static State GetState();
        static bool Initialize();
        static void Append(const Handle<Block>& blk);
        static void Accept(BlockChainVisitor* vis);
        static void Accept(BlockChainDataVisitor* vis);
        static bool HasBlock(const uint256_t& hash);
        static bool HasTransaction(const uint256_t& hash);
        static Handle<Block> GetHead();
        static Handle<Block> GetGenesis();
        static Handle<Block> GetBlock(const uint256_t& hash);
        static Handle<Block> GetBlock(uint32_t height);

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
        static void PrintBlockChain();
#endif//TOKEN_DEBUG
    };

    class BlockChainVisitor{
    public:
        virtual ~BlockChainVisitor() = default;
        virtual bool VisitStart() const{ return true; }
        virtual bool Visit(const BlockHeader& block) = 0;
        virtual bool VisitEnd() const{ return true; }
    };

    class BlockChainDataVisitor{
    protected:
        BlockChainDataVisitor() = default;
    public:
        virtual ~BlockChainDataVisitor() = default;
        virtual bool Visit(const Handle<Block>& blk) = 0;
    };
}

#endif //TOKEN_BLOCK_CHAIN_H