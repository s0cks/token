#ifndef TOKEN_UNCLAIMED_TRANSACTION_H
#define TOKEN_UNCLAIMED_TRANSACTION_H

#include "unclaimed_transaction.pb.h"

#include "json.h"
#include "object.h"
#include "builder.h"
#include "type/user.h"
#include "type/product.h"
#include "transaction_reference.h"

namespace token{
  class UnclaimedTransactionVisitor{
  protected:
    UnclaimedTransactionVisitor() = default;
  public:
    virtual ~UnclaimedTransactionVisitor() = default;
    virtual bool Visit(const UnclaimedTransactionPtr& val) = 0;
  };

  typedef internal::proto::UnclaimedTransaction RawUnclaimedTransaction;
  class UnclaimedTransaction : public BinaryObject{
  public:
    static inline int
    CompareHash(const UnclaimedTransaction& lhs, const UnclaimedTransaction& rhs){
      return Hash::Compare(lhs.hash(), rhs.hash());
    }

    struct HashComparator{
      bool operator()(const UnclaimedTransactionPtr& lhs, const UnclaimedTransactionPtr& rhs){
        return CompareHash(*lhs, *rhs) < 0;
      }
    };
  private:
    Timestamp timestamp_;
    TransactionReference source_;
    User user_;
    Product product_;
  public:
    UnclaimedTransaction():
      BinaryObject(),
      timestamp_(),
      source_(),
      user_(),
      product_(){
    }
    UnclaimedTransaction(const Timestamp& timestamp, const TransactionReference& source, const User& user, const Product& product):
      BinaryObject(),
      timestamp_(timestamp),
      source_(source),
      user_(user),
      product_(product){
    }
    explicit UnclaimedTransaction(const RawUnclaimedTransaction& raw):
      BinaryObject(),
      timestamp_(),
      source_(TransactionReference(Hash::FromHexString(raw.transaction()), raw.index())),
      user_(raw.user()),
      product_(raw.product()){
    }
    UnclaimedTransaction(const UnclaimedTransaction& rhs) = default;
    ~UnclaimedTransaction() override = default;

    Type type() const override{
      return Type::kUnclaimedTransaction;
    }

    Hash hash() const override{
      auto data = ToBuffer();
      return Hash::ComputeHash<CryptoPP::SHA256>(data->data(), data->length());
    }

    Timestamp timestamp() const{
      return timestamp_;
    }

    TransactionReference source() const{
      return source_;
    }

    User user() const{
      return user_;
    }

    Product product() const{
      return product_;
    }

    std::string ToString() const override{
      std::stringstream ss;
      ss << "UnclaimedTransaction(";
      ss << "timestamp=" << FormatTimestampReadable(timestamp_) << ", ";
      ss << "source=" << source_ << ", ";
      ss << "user=" << user_ << ", ";
      ss << "product=" << product_;
      ss << ")";
      return ss.str();
    }

    internal::BufferPtr ToBuffer() const;

    UnclaimedTransaction& operator=(const UnclaimedTransaction& rhs) = default;

    UnclaimedTransaction& operator=(const RawUnclaimedTransaction& rhs){
      return *this;
    }

    friend std::ostream& operator<<(std::ostream& stream, const UnclaimedTransaction& val){
      return stream << val.ToString();
    }

    friend bool operator==(const UnclaimedTransaction& lhs, const UnclaimedTransaction& rhs){
      return CompareHash(lhs, rhs) == 0;
    }

    friend bool operator!=(const UnclaimedTransaction& lhs, const UnclaimedTransaction& rhs){
      return CompareHash(lhs, rhs) != 0;
    }

    friend bool operator<(const UnclaimedTransaction& lhs, const UnclaimedTransaction& rhs){
      return CompareHash(lhs, rhs) < 0;//TODO: fixme
    }

    friend bool operator>(const UnclaimedTransaction& lhs, const UnclaimedTransaction& rhs){
      return CompareHash(lhs, rhs) > 0;//TODO: fixme
    }

    static inline UnclaimedTransactionPtr
    NewInstance(const Timestamp& timestamp, const TransactionReference& source, const User& user, const Product& product){
      return std::make_shared<UnclaimedTransaction>(timestamp, source, user, product);
    }

    static inline UnclaimedTransactionPtr
    From(const RawUnclaimedTransaction& val){
      return std::make_shared<UnclaimedTransaction>(val);
    }

    static inline UnclaimedTransactionPtr
    From(const internal::BufferPtr& val){
      auto length = val->GetUnsignedLong();
      RawUnclaimedTransaction raw;
      if(!val->GetMessage(raw, length)){
        return nullptr;//TODO: better error handling
      }
      return From(raw);
    }

    static inline UnclaimedTransactionPtr
    CopyFrom(const UnclaimedTransaction& val){
      return std::make_shared<UnclaimedTransaction>(val);
    }

    static inline UnclaimedTransactionPtr
    CopyFrom(const UnclaimedTransactionPtr& val){
      return CopyFrom(*val);
    }
  };

  static inline RawUnclaimedTransaction&
  operator<<(RawUnclaimedTransaction& raw, const UnclaimedTransaction& val){
    raw.set_user(val.user().data());
    raw.set_product(val.product().data());
    auto source = val.source();
    raw.set_transaction(source.transaction().HexString());
    raw.set_index(source.index());
    return raw;
  }

  static inline RawUnclaimedTransaction&
  operator<<(RawUnclaimedTransaction& raw, const UnclaimedTransactionPtr& val){
    return raw << (*val);
  }

  namespace json{
    static inline bool
    Write(Writer& writer, const UnclaimedTransactionPtr& val){
      JSON_START_OBJECT(writer);
      {
        if(!json::SetField(writer, "hash", val->hash()))
          return false;
        if(!json::SetField(writer, "timestamp", val->timestamp()))
          return false;
        if(!json::SetField(writer, "source", val->source()))
          return false;
        if(!json::SetField(writer, "user", val->user()))
          return false;
        if(!json::SetField(writer, "product", val->product()))
          return false;
      }
      JSON_END_OBJECT(writer);
      return true;
    }

    static inline bool
    SetField(Writer& writer, const char* name, const UnclaimedTransactionPtr& val){
      JSON_KEY(writer, name);
      return Write(writer, val);
    }
  }
}
#endif //TOKEN_UNCLAIMED_TRANSACTION_H