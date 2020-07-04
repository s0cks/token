#ifndef TOKEN_BLOCK_CHAIN_INDEX_H
#define TOKEN_BLOCK_CHAIN_INDEX_H

#include <leveldb/db.h>
#include "block.h"

namespace Token{
    class BlockChainIndex{
    private:
        BlockChainIndex() = delete;
    public:
        ~BlockChainIndex() = delete;

        static void Initialize();
        static void PutBlockData(Block* blk);
        static void PutReference(const std::string& name, const uint256_t& hash);
        static leveldb::DB* GetIndex();
        static Block* GetBlockData(const uint256_t& hash);
        static uint256_t GetReference(const std::string& name);
        static uint32_t GetNumberOfBlocks();
        static bool HasBlockData(const uint256_t& hash);
        static bool HasReference(const std::string& name);

        static bool HasBlockData(){
            return GetNumberOfBlocks() > 0;
        }
    };
}

#endif //TOKEN_BLOCK_CHAIN_INDEX_H