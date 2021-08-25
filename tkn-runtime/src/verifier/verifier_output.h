#ifndef TKN_VERIFIER_OUTPUT_H
#define TKN_VERIFIER_OUTPUT_H

#include "pool/pool_transaction_unclaimed.h"

namespace token{
  class OutputVerifier{
  private:
    UnclaimedTransactionPool& pool_;
  public:
    explicit OutputVerifier(UnclaimedTransactionPool& pool):
      pool_(pool){}
    ~OutputVerifier() = default;

    UnclaimedTransactionPool& GetPool() const{
      return pool_;
    }

    bool IsValid(const Output& val) const{
      NOT_IMPLEMENTED(ERROR);//TODO: implement
      return true;
    }
  };
}

#endif//TKN_VERIFIER_OUTPUT_H