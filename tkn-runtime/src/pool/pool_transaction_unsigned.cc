#include "pool/pool_transaction_unsigned.h"

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
}