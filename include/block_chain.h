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
    class BlockNode : public Object{
    private:
        BlockNode* previous_;
        BlockNode* next_;
        BlockHeader value_;

        BlockNode(const BlockHeader& value):
            Object(),
            previous_(nullptr),
            next_(nullptr),
            value_(value){
            SetType(Type::kBlockNodeType);
        }
    protected:
        bool Accept(WeakObjectPointerVisitor* vis){
            if(HasNext() && !vis->Visit(&next_)){
                LOG(WARNING) << "couldn't visit next_";
                return false;
            }
            return true;
        }
    public:
        bool HasPrevious() const{
            return previous_ != nullptr;
        }

        Handle<BlockNode> GetPrevious() const{
            return previous_;
        }

        void SetPrevious(const Handle<BlockNode>& node){
            WriteBarrier(&previous_, node);
        }

        bool HasNext() const{
            return next_ != nullptr;
        }

        Handle<BlockNode> GetNext() const{
            return next_;
        }

        void SetNext(const Handle<BlockNode>& next){
            WriteBarrier(&next_, next);
        }

        BlockHeader GetValue() const{
            return value_;
        }

        std::string ToString() const{
            std::stringstream ss;
            ss << "BlockNode(" << GetValue() << ")";
            return ss.str();
        }

        static Handle<BlockNode> NewInstance(const BlockHeader& blk){
            return new BlockNode(blk);
        }

        static Handle<BlockNode> NewInstance(const Handle<Block>& blk){
            return new BlockNode(blk->GetHeader());
        }
    };

    class BlockChainVisitor;
    class BlockChainNodeVisitor;
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
        static void SetHeadNode(const Handle<BlockNode>& node);
        static void SetGenesisNode(const Handle<BlockNode>& node);
        static Handle<BlockNode> GetHeadNode();
        static Handle<BlockNode> GetGenesisNode();
        static Handle<BlockNode> GetNode(const Hash& hash);
    public:
        ~BlockChain() = delete;

        static State GetState();
        static bool Initialize();
        static bool Print(bool is_detailed=false);
        static bool Accept(BlockChainVisitor* vis);
        static bool Accept(BlockChainDataVisitor* vis);
        static bool Accept(BlockChainNodeVisitor* vis);
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

    class BlockChainNodeVisitor{
    protected:
        BlockChainNodeVisitor() = default;
    public:
        virtual ~BlockChainNodeVisitor() = default;
        virtual bool VisitStart() const{ return true; }
        virtual bool Visit(const Handle<BlockNode>& node) = 0;
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