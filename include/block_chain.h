#ifndef TOKEN_BLOCK_CHAIN_H
#define TOKEN_BLOCK_CHAIN_H

#include <leveldb/db.h>
#include <map>

#include "common.h"
#include "block.h"
#include "utxo.h"

namespace Token{
    class BlockChainVisitor{
    public:
        BlockChainVisitor(){}
        virtual ~BlockChainVisitor() = default;

        virtual bool VisitStart() = 0;
        virtual bool Visit(Block* block) = 0;
        virtual bool VisitEnd() = 0;
    };

    class BlockChain{
    public:
        static const uintptr_t kBlockChainInitSize = 1024;
    private:
        std::string path_;
        pthread_rwlock_t lock_;
        leveldb::DB* state_;
        std::vector<Block*> blocks_;

        static BlockChain* GetInstance(); //TODO: Revoke access

        static inline leveldb::DB*
        GetState(){
            return GetInstance()->state_;
        }

        bool CreateGenesis(); //TODO: Remove

        bool LoadBlockChain(const std::string& path);
        bool InitializeChainState(const std::string& root);

        bool GetReference(const std::string& ref, uint32_t* height); //TODO: Remove
        bool SetReference(const std::string& ref, uint32_t height); //TODO: Remove

        bool SaveBlock(Block* block);
        bool LoadBlock(uint32_t height, Block* result);

        static bool AppendBlock(Block* block);

        BlockChain():
            lock_(),
            path_(),
            state_(nullptr),
            blocks_(){
        }

        friend class BlockMiner; // TODO: Remove
    public:
        static void Accept(BlockChainVisitor* vis);
        static Block* GetHead();
        static Block* GetGenesis();
        static Block* GetBlock(const std::string& hash);
        static Block* GetBlock(uint32_t height);
        static uint32_t GetHeight();
        static bool GetBlockList(std::vector<std::string>& blocks);
        static bool HasHead();
        static bool ContainsBlock(const std::string& hash);
        static bool Initialize();
    };
}

#endif //TOKEN_BLOCK_CHAIN_H