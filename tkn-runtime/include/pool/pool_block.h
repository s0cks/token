#ifndef TKN_BLOCK_POOL_H
#define TKN_BLOCK_POOL_H

#include "block.h"
#include "pool/pool.h"

namespace token{
  class BlockPool : public internal::ObjectPool<Block>{
  public:
    explicit BlockPool(const std::string& filename):
      internal::ObjectPool<Block>(Type::kBlock, filename + "/blocks"){}
    ~BlockPool() override = default;
    bool Visit(BlockVisitor* vis) const;
  };
}

#endif//TKN_BLOCK_POOL_H