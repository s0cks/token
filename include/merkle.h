#ifndef TOKEN_MERKLE_H
#define TOKEN_MERKLE_H

#include "common.h"
#include <vector>

namespace Token{
    class Block;

    std::string GetMerkleRoot(std::vector<std::string>& leaves);
    std::string GetBlockMerkleRoot(Block* block);
}

#endif //TOKEN_MERKLE_H