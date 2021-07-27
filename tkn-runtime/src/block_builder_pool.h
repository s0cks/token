#ifndef TKN_BLOCK_BUILDER_POOL_H
#define TKN_BLOCK_BUILDER_POOL_H

#include "block_builder.h"

namespace token{
  class PoolBlockBuilder : public BlockBuilder{
  public:
    PoolBlockBuilder() = default;
    ~PoolBlockBuilder() override = default;
    BlockPtr Build() const override;
  };
}

#endif//TKN_BLOCK_BUILDER_POOL_H