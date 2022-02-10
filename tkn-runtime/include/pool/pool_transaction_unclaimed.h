#ifndef TKN_UNCLAIMED_TRANSACTION_POOL_H
#define TKN_UNCLAIMED_TRANSACTION_POOL_H

#include "pool.h"
#include "../../../Sources/token/transaction_unclaimed.h"

namespace token{
  class UnclaimedTransactionPool : public internal::ObjectPool<UnclaimedTransaction>{
  public:
    explicit UnclaimedTransactionPool(const std::string& filename):
      internal::ObjectPool<UnclaimedTransaction>(Type::kUnclaimedTransaction, filename + "/unclaimed_transactions"){}
    ~UnclaimedTransactionPool() override = default;
    bool Visit(UnclaimedTransactionVisitor* vis) const;
  };
}

#endif//TKN_UNCLAIMED_TRANSACTION_POOL_H