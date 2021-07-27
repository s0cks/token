#ifndef TOKEN_SIGNED_TRANSACTION_H
#define TOKEN_SIGNED_TRANSACTION_H

#include "transaction.h"

namespace token{
  class SignedTransaction : public internal::TransactionBase{
  public:
  class Encoder : public codec::TransactionEncoder<SignedTransaction> {
    public:
      explicit Encoder(const SignedTransaction* value, const codec::EncoderFlags &flags = codec::kDefaultEncoderFlags):
        codec::TransactionEncoder<SignedTransaction>(value, flags) {}
      Encoder(const Encoder &other) = default;
      ~Encoder() override = default;
      int64_t GetBufferSize() const override;
      bool Encode(const BufferPtr &buff) const override;
      Encoder &operator=(const Encoder &other) = default;
    };

  class Decoder : public codec::TransactionDecoder<SignedTransaction>{
    public:
      explicit Decoder(const codec::DecoderHints& hints);
      Decoder(const Decoder& other) = default;
      ~Decoder() override = default;
      bool Decode(const BufferPtr &buff, SignedTransaction& result) const override;
      Decoder& operator=(const Decoder& other) = default;
    };
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