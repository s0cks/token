#ifndef TOKEN_UNSIGNED_TRANSACTION_H
#define TOKEN_UNSIGNED_TRANSACTION_H

#include "transaction.h"

namespace token{
  class UnsignedTransaction : public internal::TransactionBase{
 public:
    struct TimestampComparator{
      bool operator()(const UnsignedTransactionPtr& a, const UnsignedTransactionPtr& b){
        return a->timestamp() < b->timestamp();
      }
    };

  class Encoder : public TransactionEncoder<UnsignedTransaction>{
   public:
    explicit Encoder(const UnsignedTransaction& value, const codec::EncoderFlags& flags=codec::kDefaultEncoderFlags):
        TransactionEncoder<UnsignedTransaction>(value, flags){}
    Encoder(const Encoder& other) = default;
    ~Encoder() override = default;
    Encoder& operator=(const Encoder& other) = default;
  };

  class Decoder : public TransactionDecoder<UnsignedTransaction>{
   public:
    explicit Decoder(const codec::DecoderHints& hints=codec::kDefaultDecoderHints):
        TransactionDecoder<UnsignedTransaction>(hints){}
    Decoder(const Decoder& other) = default;
    ~Decoder() override = default;
    bool Decode(const BufferPtr& buff, UnsignedTransaction& result) const override;
    Decoder& operator=(const Decoder& other) = default;
  };
 public:
  UnsignedTransaction():
    internal::TransactionBase(){}
  UnsignedTransaction(const Timestamp& timestamp, const InputList& inputs, const OutputList& outputs):
    internal::TransactionBase(timestamp, inputs, outputs){}
  UnsignedTransaction(const UnsignedTransaction& other) = default;
  ~UnsignedTransaction() override = default;

  Type type() const override{
    return Type::kUnsignedTransaction;
  }

  BufferPtr ToBuffer() const override;
  std::string ToString() const override;

  UnsignedTransaction& operator=(const UnsignedTransaction& other) = default;

  static inline UnsignedTransactionPtr
  NewInstance(const Timestamp& timestamp, const InputList& inputs, const OutputList& outputs){
    return std::make_shared<UnsignedTransaction>(timestamp, inputs, outputs);
  }

  static inline bool
  Decode(const BufferPtr& buff, UnsignedTransaction& result, const codec::DecoderHints& hints=codec::kDefaultDecoderHints){
    Decoder decoder(hints);
    return decoder.Decode(buff, result);
  }

  static inline UnsignedTransactionPtr
  DecodeNew(const BufferPtr& buff, const codec::DecoderHints& hints=codec::kDefaultDecoderHints){
    UnsignedTransaction instance;
    if(!Decode(buff, instance, hints))
      return nullptr;
    return std::make_shared<UnsignedTransaction>(instance);
  }
};

  typedef std::vector<UnsignedTransactionPtr> UnsignedTransactionList;

  namespace json{
    static inline bool
    SetField(Writer& writer, const char* name, const UnsignedTransactionPtr& val){
      NOT_IMPLEMENTED(FATAL);
      return false;
    }
  }
}

#endif//TOKEN_UNSIGNED_TRANSACTION_H