#ifndef TOKEN_BLOCK_POOL_INDEX_H
#define TOKEN_BLOCK_POOL_INDEX_H

#include "block.h"

namespace Token{
    class BlockPoolIndex{
    public:
        BlockPoolIndex() = delete;
        ~BlockPoolIndex() = delete;

        static void Initialize();
        static Handle<Block> GetData(const uint256_t& hash);
        static size_t GetNumberOfBlocks();
        static bool PutData(const Handle<Block>& utxo);
        static bool HasData(const uint256_t& hash);

        static inline bool HasData(){
            return GetNumberOfBlocks() > 0;
        }

        static std::string GetDirectory();
    };
}

#endif //TOKEN_BLOCK_POOL_INDEX_H