#include "object_pool.h"

namespace token{
  bool UnsignedTransactionPool::Visit(UnsignedTransactionVisitor* vis) const{
    Iterator iterator(index());
    while(iterator.HasNext()){
      auto val = iterator.Next();
      DVLOG(2) << "visiting unsigned transaction " << val->hash() << "....";
      if(!vis->Visit(val))
        return false;
    }
    return true;
  }

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

  bool UnclaimedTransactionPool::Visit(UnclaimedTransactionVisitor* vis) const{
    Iterator iterator(index());
    while(iterator.HasNext()){
      auto val = iterator.Next();
      DVLOG(2) << "visiting unclaimed transaction " << val->hash() << "....";
      if(!vis->Visit(val))
        return false;
    }
    return true;
  }
}