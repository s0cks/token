#ifndef TKN_INPUT_VERIFIER_H
#define TKN_INPUT_VERIFIER_H

#include "object_pool.h"

namespace token{
  class InputVerifier{
  private:
    UnclaimedTransactionPool& pool_;
  public:
    explicit InputVerifier(UnclaimedTransactionPool& pool):
      pool_(pool){}
    ~InputVerifier() = default;

    UnclaimedTransactionPool& GetPool() const{
      return pool_;
    }

    bool IsValid(const Input& val) const{
      return IsValid(val.hash());
    }

    bool IsValid(const Hash& hash) const{
      if(!GetPool().Has(hash)){
        LOG(ERROR) << hash << " is not valid, cannot find unclaimed transaction in pool.";
        return false;
      }
      DVLOG(1) << hash << " is valid.";
      return true;
    }
  };
}

#endif//TKN_INPUT_VERIFIER_H