#ifndef TOKEN_UNSIGNED_TRANSACTION_H
#define TOKEN_UNSIGNED_TRANSACTION_H

#include <utility>

#include "transaction.h"

namespace token{
  class UnsignedTransaction: public internal::TransactionBase<internal::proto::UnsignedTransaction>{
  public:
    struct TimestampComparator{
      bool operator()(const UnsignedTransactionPtr& a, const UnsignedTransactionPtr& b){
        return a->timestamp() < b->timestamp();
      }
    };

    class Builder : public TransactionBuilderBase<UnsignedTransaction>{
    public:
      explicit Builder(internal::proto::UnsignedTransaction* raw):
        TransactionBuilderBase<UnsignedTransaction>(raw){}
      Builder() = default;
      Builder(const Builder& rhs) = default;
      ~Builder() override = default;

      UnsignedTransactionPtr Build() const override{
        return std::make_shared<UnsignedTransaction>(*raw_);
      }

      Builder& operator=(const Builder& rhs) = default;
    };
  public:
    UnsignedTransaction():
      internal::TransactionBase<internal::proto::UnsignedTransaction>(){}
    explicit UnsignedTransaction(internal::proto::UnsignedTransaction raw):
      internal::TransactionBase<internal::proto::UnsignedTransaction>(std::move(raw)){}
    explicit UnsignedTransaction(const internal::BufferPtr& data):
      UnsignedTransaction(){
      if(!raw_.ParseFromArray(data->data(), static_cast<int>(data->length())))
        DLOG(FATAL) << "cannot parse UnsignedTransaction from buffer.";
    }
    UnsignedTransaction(const UnsignedTransaction& other) = default;
    ~UnsignedTransaction() override = default;

    Type type() const override{
      return Type::kUnsignedTransaction;
    }

    Hash hash() const override{
      auto data = ToBuffer();
      return Hash::ComputeHash<CryptoPP::SHA256>(data->data(), data->length());
    }

    BufferPtr ToBuffer() const;
    std::string ToString() const override;
    UnsignedTransaction& operator=(const UnsignedTransaction& other) = default;

    friend bool operator==(const UnsignedTransaction& lhs, const UnsignedTransaction& rhs){
      return lhs.hash() == rhs.hash();
    }

    friend bool operator!=(const UnsignedTransaction& lhs, const UnsignedTransaction& rhs){
      return lhs.hash() != rhs.hash();
    }

    static inline UnsignedTransactionPtr
    Decode(const BufferPtr& data){
      return std::make_shared<UnsignedTransaction>(data);
    }
  };

  namespace json{
    static inline bool
    SetField(Writer& writer, const char* name, const UnsignedTransactionPtr& val){
      NOT_IMPLEMENTED(FATAL);
      return false;
    }
  }
}
#endif//TOKEN_UNSIGNED_TRANSACTION_H