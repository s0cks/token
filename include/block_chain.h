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
#define FOR_EACH_BLOCKCHAIN_STATE(V) \
    V(Uninitialized)                 \
    V(Initializing)                  \
    V(Initialized)                   \
    V(Synchronizing)                 \

#define FOR_EACH_BLOCKCHAIN_STATUS(V) \
    V(Ok)                             \
    V(Warning)                        \
    V(Error)

#define BLOCKCHAIN_REFERENCE_GENESIS "<GENESIS>"
#define BLOCKCHAIN_REFERENCE_HEAD "<HEAD>"
    class BlockChainVisitor;
    class BlockChain{
        friend class Server;
        friend class ProposalHandler; //TODO: revoke access?
        friend class SynchronizeBlockChainTask; //TODO: revoke access
        friend class BlockDiscoveryThread; //TODO: revoke access
    public:
        enum State{
#define DEFINE_STATE(Name) k##Name,
            FOR_EACH_BLOCKCHAIN_STATE(DEFINE_STATE)
#undef DEFINE_STATE
        };

        enum Status{
#define DEFINE_STATUS(Name) k##Name,
            FOR_EACH_BLOCKCHAIN_STATUS(DEFINE_STATUS)
#undef DEFINE_STATUS
        };
    private:
        BlockChain() = delete;
        static leveldb::DB* GetIndex();
        static void SetState(State state);
        static void SetStatus(Status status);
        static bool PutBlock(const Hash& hash, Block* blk);
        static bool PutReference(const std::string& name, const Hash& hash);
        static bool RemoveReference(const std::string& name);
        static bool Append(Block* blk);
    public:
        ~BlockChain() = delete;

        static State GetState();
        static Status GetStatus();
        static std::string GetStatusMessage();
        static bool Initialize();
        static bool HasBlock(const Hash& hash);
        static bool Accept(BlockChainVisitor* vis);
        static bool Print(bool is_detailed=false);
        static bool HasReference(const std::string& name);
        static Hash GetReference(const std::string& name);
        static Block* GetBlock(const Hash& hash);
        static Block* GetBlock(int64_t height);
        static Block* GetHead();
        static Block* GetGenesis();
        static int64_t GetNumberOfBlocks();

        static inline bool HasBlocks(){
            return GetNumberOfBlocks() > 0;
        }

        static inline bool
        HasHead(){
            return !GetReference(BLOCKCHAIN_REFERENCE_HEAD).IsNull();
        }

        static inline bool
        HasGenesis(){
            return !GetReference(BLOCKCHAIN_REFERENCE_GENESIS).IsNull();
        }

#define DEFINE_STATE_CHECK(Name) \
        static inline bool Is##Name(){ return GetState() == State::k##Name; }
        FOR_EACH_BLOCKCHAIN_STATE(DEFINE_STATE_CHECK)
#undef DEFINE_STATE_CHECK

#define DEFINE_STATUS_CHECK(Name) \
        static inline bool Is##Name(){ return GetStatus() == Status::k##Name; }
        FOR_EACH_BLOCKCHAIN_STATUS(DEFINE_STATUS_CHECK)
#undef DEFINE_STATUS_CHECK
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