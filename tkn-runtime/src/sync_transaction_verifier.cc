#include "sync_transaction_verifier.h"

namespace token{
  namespace sync{
    bool TransactionVerifier::Visit(const Input& val){
      NOT_IMPLEMENTED(FATAL);//TODO: implement
      return false;
    }

    bool TransactionVerifier::Visit(const Output& val){
      NOT_IMPLEMENTED(FATAL);//TODO: implement
      return false;
    }
  }
}