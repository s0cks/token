#ifndef TKN_POOL_TRANSACTION_UNSIGNED_H
#define TKN_POOL_TRANSACTION_UNSIGNED_H

#include "pool/pool.h"
#include "transaction_unsigned.h"

namespace token{
  class UnsignedTransactionPool : public internal::ObjectPool<UnsignedTransaction>{
  public:
    explicit UnsignedTransactionPool(const std::string& filename):
      internal::ObjectPool<UnsignedTransaction>(Type::kUnsignedTransaction, filename + "/unsigned_transactions"){}
    ~UnsignedTransactionPool() override = default;
    bool Visit(UnsignedTransactionVisitor* vis) const;
  };
}

#endif//TKN_POOL_TRANSACTION_UNSIGNED_H