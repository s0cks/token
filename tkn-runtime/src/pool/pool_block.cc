#include "pool/pool_block.h"

namespace token{
  bool BlockPool::Visit(BlockVisitor* vis) const{
    Iterator iterator(index());
    while(iterator.HasNext()){
      auto val = iterator.Next();
      DVLOG(2) << "visiting block " << val->hash() << "....";
      if(!vis->Visit(val))
        return false;
    }
    return true;
  }
}