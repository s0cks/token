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
#define BLOCKCHAIN_REFERENCE_GENESIS "<GENESIS>"
#define BLOCKCHAIN_REFERENCE_HEAD "<HEAD>"

    class BlockChainVisitor;
    class BlockChain{
        friend class Server;
        friend class SynchronizeBlockChainTask; //TODO: revoke access
        friend class BlockDiscoveryThread; //TODO: revoke access
    public:
        enum State{
            kUninitialized,
            kInitializing,
            kInitialized
        };
    private:
        BlockChain() = delete;
        static leveldb::DB* GetIndex();
        static void SetState(State state);
        static bool PutBlock(const Hash& hash, const Handle<Block>& blk);
        static bool PutReference(const std::string& name, const Hash& hash);
        static Hash GetReference(const std::string& name);
        static bool HasReference(const std::string& name);
        static bool Append(const Handle<Block>& blk);
    public:
        ~BlockChain() = delete;

        static State GetState();
        static bool Initialize();
        static bool HasBlock(const Hash& hash);
        static Handle<Block> GetBlock(const Hash& hash);
        static Handle<Block> GetBlock(int64_t height);
        static Handle<Block> GetHead();
        static Handle<Block> GetGenesis();
        static bool Accept(BlockChainVisitor* vis);
        static bool Print(bool is_detailed=false);
        static int64_t GetNumberOfBlocks();

        static inline bool HasBlocks(){
            return GetNumberOfBlocks() > 0;
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

    class BlockChainVisitor{
    protected:
        BlockChainVisitor() = default;
    public:
        virtual ~BlockChainVisitor() = default;
        virtual bool VisitStart() const{ return true; }
        virtual bool Visit(const BlockHeader& blk) const = 0;
        virtual bool VisitEnd() const{ return true; }
    };
}

#endif //TOKEN_BLOCK_CHAIN_H