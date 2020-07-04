#ifndef TOKEN_BLOCK_CHAIN_H
#define TOKEN_BLOCK_CHAIN_H

#include <leveldb/db.h>
#include <map>

#include "common.h"
#include "merkle.h"
#include "pool.h"
#include "block.h"
#include "transaction.h"
#include "unclaimed_transaction.h"

namespace Token{
    class BlockNode;
    class BlockChainVisitor;
    class BlockChain{
    private:
        friend class BlockChainServer;
        friend class BlockMiner;

        BlockNode* genesis_;
        BlockNode* head_;
        std::map<uint32_t, BlockNode*> blocks_;
        std::map<uint256_t, BlockNode*> nodes_; //TODO: possible memory leak?

        static BlockChain* GetInstance();
        bool Append(Block* block);

        static BlockNode* GetHeadNode();
        static BlockNode* GetGenesisNode();
        static BlockNode* GetNode(const uint256_t& hash);
        static BlockNode* GetNode(uint32_t height);

        BlockChain();
    public:
        ~BlockChain(){}

        static uint32_t GetHeight(){
            return GetHead().GetHeight();
        }

        static BlockHeader GetHead();
        static BlockHeader GetGenesis();
        static BlockHeader GetBlock(const uint256_t& hash);
        static BlockHeader GetBlock(uint32_t height);

        static bool ContainsBlock(const uint256_t& hash){
            return GetInstance()->GetNode(hash) != nullptr;
        }

        static Block* GetBlockData(const uint256_t& hash);
        static bool Initialize();
        static bool Accept(BlockChainVisitor* vis);

        //TODO remove
        static bool AppendBlock(Block* block){
            return GetInstance()->Append(block);
        }
    };

    class BlockChainVisitor{
    public:
        virtual ~BlockChainVisitor() = default;
        virtual bool VisitStart() const{ return true; }
        virtual bool Visit(const BlockHeader& block) const = 0;
        virtual bool VisitEnd() const{ return true; }
    };
}

#endif //TOKEN_BLOCK_CHAIN_H