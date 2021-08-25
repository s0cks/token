#include "pool/pool_transaction_unclaimed.h"

namespace token{
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