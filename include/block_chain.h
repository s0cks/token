#ifndef TOKEN_BLOCK_CHAIN_H
#define TOKEN_BLOCK_CHAIN_H

#include <map>
#include <set>
#include <leveldb/db.h>

#include "common.h"
#include "merkle.h"
#include "block.h"
#include "transaction.h"
#include "unclaimed_transaction.h"

namespace Token{
    class BlockNode;
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

        static void SetState(State state);
        static void SetHeadNode(BlockNode* node);
        static void SetGenesisNode(BlockNode* node);
        static BlockNode* GetHeadNode();
        static BlockNode* GetGenesisNode();
        static BlockNode* GetNode(const Hash& hash);
    public:
        ~BlockChain() = delete;

        static State GetState();
        static bool Initialize();
        static bool Print(bool is_detailed=false);
        static bool Accept(BlockChainVisitor* vis);
        static bool Accept(BlockChainDataVisitor* vis);
        static void Append(const Handle<Block>& blk);
        static bool HasBlock(const Hash& hash);
        static bool HasTransaction(const Hash& hash);
        static BlockHeader GetHead();
        static BlockHeader GetGenesis();
        static Handle<Block> GetBlock(const Hash& hash);
        static Handle<Block> GetBlock(uint32_t height);

        static bool GetHeaders(std::set<BlockHeader>& blocks);

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