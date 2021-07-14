#ifndef TOKEN_SIGNED_TRANSACTION_H
#define TOKEN_SIGNED_TRANSACTION_H

#include "transaction.h"

namespace token{
  namespace codec{
    class SignedTransactionEncoder : public TransactionEncoder<SignedTransaction> {
     public:
      explicit SignedTransactionEncoder(const SignedTransaction& value, const codec::EncoderFlags &flags = codec::kDefaultEncoderFlags):
        TransactionEncoder<SignedTransaction>(value, flags) {}
      SignedTransactionEncoder(const SignedTransactionEncoder &other) = default;
      ~SignedTransactionEncoder() override = default;
      int64_t GetBufferSize() const override;
      bool Encode(const BufferPtr &buff) const override;
      SignedTransactionEncoder &operator=(const SignedTransactionEncoder &other) = default;
    };

    class SignedTransactionDecoder : public TransactionDecoder<SignedTransaction>{
     public:
      explicit SignedTransactionDecoder(const codec::DecoderHints& hints);
      SignedTransactionDecoder(const SignedTransactionDecoder& other) = default;
      ~SignedTransactionDecoder() override = default;
      bool Decode(const BufferPtr &buff, SignedTransaction& result) const override;
      SignedTransactionDecoder& operator=(const SignedTransactionDecoder& other) = default;
    };
  }

  class SignedTransaction : public internal::TransactionBase{
   public:
    SignedTransaction(const Timestamp& timestamp, const InputList& inputs, const OutputList& outputs):
      internal::TransactionBase(timestamp, inputs, outputs){}
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