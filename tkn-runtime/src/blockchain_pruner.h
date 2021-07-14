#ifndef TOKEN_BLOCKCHAIN_PRUNER_H
#define TOKEN_BLOCKCHAIN_PRUNER_H

#include "blockchain.h"

namespace token{
  class BlockChainPruner : public BlockChainBlockVisitor{
   protected:
    HashSet seen_;
    HashSet pruned_;
    HashSet live_;

    BlockChainPruner():
      BlockChainBlockVisitor(),
      seen_(),
      pruned_(),
      live_(){}
   public:
    virtual ~BlockChainPruner() = default;

    HashSet& seen(){
      return seen_;
    }

    HashSet seen() const{
      return seen_;
    }

    HashSet& pruned(){
      return pruned_;
    }

    HashSet pruned() const{
      return pruned_;
    }

    HashSet& live(){
      return live_;
    }

    HashSet live() const{
      return live_;
    }

    virtual bool ShouldPrune(const Hash& hash, const BlockPtr& blk) const = 0;

    bool Visit(const BlockPtr& blk){
      Hash hash = blk->GetHash();
      if(!seen_.insert(hash).second)
        return false;

      if(ShouldPrune(hash, blk)){
        if(!pruned_.insert(hash))
          return false;
        return BlockChain::RemoveBlock(hash, blk);
      }
      return live_.insert(hash).second;
    }
  };
}

#endif//TOKEN_BLOCKCHAIN_PRUNER_H