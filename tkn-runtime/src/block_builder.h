#ifndef TKN_BLOCK_BUILDER_H
#define TKN_BLOCK_BUILDER_H

#include "block.h"

namespace token{
  class BlockBuilder{
  protected:
    BlockBuilder() = default;
  public:
    virtual ~BlockBuilder() = default;
    virtual BlockPtr Build() const = 0;
  };

  // for testing
  class GenesisBlockBuilder : public BlockBuilder{
  public:
    GenesisBlockBuilder():
      BlockBuilder(){}
    ~GenesisBlockBuilder() override = default;

    BlockPtr Build() const override{
      return Block::NewGenesis();
    }
  };
}

#endif//TKN_BLOCK_BUILDER_H