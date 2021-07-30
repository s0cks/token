#ifndef TOKEN_SIGNED_TRANSACTION_H
#define TOKEN_SIGNED_TRANSACTION_H

#include <utility>

#include "transaction.h"

namespace token{
  class SignedTransaction : public internal::TransactionBase<internal::proto::SignedTransaction>{
   public:
    SignedTransaction():
      internal::TransactionBase<internal::proto::SignedTransaction>(){}
    explicit SignedTransaction(internal::proto::SignedTransaction raw):
      internal::TransactionBase<internal::proto::SignedTransaction>(std::move(raw)){}
    SignedTransaction(const Timestamp& timestamp, const std::vector<Input>& inputs, const std::vector<Output>& outputs):
      internal::TransactionBase<internal::proto::SignedTransaction>(timestamp, inputs, outputs){}
    SignedTransaction(const SignedTransaction& other) = default;
    ~SignedTransaction() override = default;

    Type type() const override{
      return Type::kSignedTransaction;
    }

    Hash hash() const override{
      return Hash();
    }

    std::string ToString() const override;

    SignedTransaction& operator=(const SignedTransaction& other) = default;
  };

  namespace json{
    static inline bool
    SetField(Writer& writer, const char* name, const SignedTransactionPtr& val){
      NOT_IMPLEMENTED(FATAL);
      return false;
    }
  }
}

#endif//TOKEN_SIGNED_TRANSACTION_H