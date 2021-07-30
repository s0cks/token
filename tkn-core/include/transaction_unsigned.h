#ifndef TOKEN_UNSIGNED_TRANSACTION_H
#define TOKEN_UNSIGNED_TRANSACTION_H

#include "transaction.h"

namespace token{
  class UnsignedTransaction: public internal::TransactionBase{
  public:
    struct TimestampComparator{
      bool operator()(const UnsignedTransactionPtr& a, const UnsignedTransactionPtr& b){
        return a->timestamp() < b->timestamp();
      }
    };

  class Encoder: public TransactionEncoder<UnsignedTransaction>{
    public:
      explicit Encoder(const UnsignedTransaction* value, const codec::EncoderFlags& flags = codec::kDefaultEncoderFlags):
        TransactionEncoder<UnsignedTransaction>(value, flags){}
      Encoder(const Encoder& other) = default;
      ~Encoder() override = default;
      int64_t GetBufferSize() const override;
      bool Encode(const BufferPtr& buff) const override;
      Encoder& operator=(const Encoder& other) = default;
    };

  class Decoder: public TransactionDecoder<UnsignedTransaction>{
    public:
      explicit Decoder(const codec::DecoderHints& hints):
          TransactionDecoder<UnsignedTransaction>(hints){}
      ~Decoder() override = default;
      UnsignedTransaction* Decode(const BufferPtr& data) const override;
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

    Hash hash() const override{
      return Hash();
    }

    BufferPtr ToBuffer() const;
    std::string ToString() const override;
    UnsignedTransaction& operator=(const UnsignedTransaction& other) = default;

    static inline UnsignedTransactionPtr
    NewInstance(const Timestamp& timestamp, const InputList& inputs, const OutputList& outputs){
      return std::make_shared<UnsignedTransaction>(timestamp, inputs, outputs);
    }

    static inline UnsignedTransactionPtr
    Decode(const BufferPtr& data, const codec::DecoderHints& hints=codec::kDefaultDecoderHints){
      Decoder decoder(hints);

      UnsignedTransaction* value = nullptr;
      if(!(value = decoder.Decode(data))){
        DLOG(FATAL) << "cannot decode UnsignedTransaction from buffer.";
        return nullptr;
      }
      return std::shared_ptr<UnsignedTransaction>(value);
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