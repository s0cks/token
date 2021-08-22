#ifndef TOKEN_UNSIGNED_TRANSACTION_H
#define TOKEN_UNSIGNED_TRANSACTION_H

#include <utility>

#include "transaction.h"

namespace token{
  class UnsignedTransactionVisitor{
  protected:
    UnsignedTransactionVisitor() = default;
  public:
    virtual ~UnsignedTransactionVisitor() = default;
    virtual bool Visit(const UnsignedTransactionPtr& val) = 0;
  };

  typedef internal::proto::UnsignedTransaction RawUnsignedTransaction;
  class UnsignedTransaction : public internal::TransactionBase{
  public:
    static inline int
    CompareHash(const UnsignedTransaction& lhs, const UnsignedTransaction& rhs){
      return Hash::Compare(lhs.hash(), rhs.hash());
    }

    struct HashComparator{
      bool operator()(const UnsignedTransactionPtr& lhs, const UnsignedTransactionPtr& rhs){
        return CompareHash(*lhs, *rhs) < 0;
      }
    };

    static inline int
    CompareTimestamp(const UnsignedTransaction& lhs, const UnsignedTransaction& rhs){
      if(lhs.timestamp() < rhs.timestamp()){
        return -1;
      } else if(lhs.timestamp() > rhs.timestamp()){
        return +1;
      }
      return 0;
    }

    struct TimestampComparator{
      bool operator()(const UnsignedTransactionPtr& lhs, const UnsignedTransactionPtr& rhs){
        return CompareTimestamp(*lhs, *rhs) < 0;
      }
    };
  public:
    UnsignedTransaction() = default;
    UnsignedTransaction(const Timestamp& timestamp, const InputList& inputs, const OutputList& outputs):
      internal::TransactionBase(timestamp, inputs, outputs){
    }
    explicit UnsignedTransaction(const RawUnsignedTransaction& raw):
      internal::TransactionBase(raw){
    }
    UnsignedTransaction(const UnsignedTransaction& rhs) = default;
    ~UnsignedTransaction() override = default;

    Type type() const override{
      return Type::kUnsignedTransaction;
    }

    Hash hash() const override{
      auto data = ToBuffer();
      return Hash::ComputeHash<CryptoPP::SHA256>(data->data(), data->length());
    }

    std::string ToString() const override{
      std::stringstream ss;
      ss << "UnsignedTransaction(";
      ss << "hash=" << hash();
      ss << ")";//TODO: fix implementation
      return ss.str();
    }

    internal::BufferPtr ToBuffer() const;

    UnsignedTransaction& operator=(const UnsignedTransaction& rhs) = default;

    UnsignedTransaction& operator=(const RawUnsignedTransaction& rhs){
      NOT_IMPLEMENTED(ERROR);//TODO: implement
      return *this;
    }

    friend std::ostream& operator<<(std::ostream& stream, const UnsignedTransaction& rhs){
      return stream << rhs.ToString();
    }

    friend bool operator==(const UnsignedTransaction& lhs, const UnsignedTransaction& rhs){
      return CompareHash(lhs, rhs) == 0;
    }

    friend bool operator!=(const UnsignedTransaction& lhs, const UnsignedTransaction& rhs){
      return CompareHash(lhs, rhs) != 0;
    }

    friend bool operator<(const UnsignedTransaction& lhs, const UnsignedTransaction& rhs){
      return CompareTimestamp(lhs, rhs) < 0;
    }

    friend bool operator>(const UnsignedTransaction& lhs, const UnsignedTransaction& rhs){
      return CompareTimestamp(lhs, rhs) > 0;
    }

    static inline UnsignedTransactionPtr
    NewInstance(const Timestamp& timestamp, const InputList& inputs, const OutputList& outputs){
      return std::make_shared<UnsignedTransaction>(timestamp, inputs, outputs);
    }

    static inline UnsignedTransactionPtr
    From(const RawUnsignedTransaction& val){
      return std::make_shared<UnsignedTransaction>(val);
    }

    static inline UnsignedTransactionPtr
    From(const internal::BufferPtr& val){
      auto length = val->GetUnsignedLong();
      RawUnsignedTransaction raw;
      if(!val->GetMessage(raw, length)){
        LOG(FATAL) << "cannot decode UnsignedTransaction (" << PrettySize(length) << ") from " << val->ToString();
        return nullptr;
      }
      return From(raw);
    }

    static inline UnsignedTransactionPtr
    CopyFrom(const UnsignedTransaction& val){
      return std::make_shared<UnsignedTransaction>(val);
    }

    static inline UnsignedTransactionPtr
    CopyFrom(const UnsignedTransactionPtr& val){
      return CopyFrom(*val);
    }
  };

  static inline RawUnsignedTransaction&
  operator<<(RawUnsignedTransaction& raw, const UnsignedTransaction& val){
    raw.set_timestamp(ToUnixTimestamp(val.timestamp()));
    for(auto iter = val.inputs_begin(); iter != val.inputs_end(); iter++)
      (*raw.add_inputs()) << (*iter);
    for(auto iter = val.outputs_begin(); iter != val.outputs_end(); iter++)
      (*raw.add_outputs()) << (*iter);
    return raw;
  }

  static inline RawUnsignedTransaction&
  operator<<(RawUnsignedTransaction& raw, const UnsignedTransactionPtr& val){
    return raw << (*val);
  }

  typedef std::vector<UnsignedTransactionPtr> UnsignedTransactionList;

  static inline UnsignedTransactionList&
  operator<<(UnsignedTransactionList& list, const UnsignedTransactionPtr& val){
    list.push_back(val);
    return list;
  }

  static inline UnsignedTransactionList&
  operator<<(UnsignedTransactionList& list, const UnsignedTransaction& val){
    return list << UnsignedTransaction::CopyFrom(val);
  }

  typedef std::set<UnsignedTransactionPtr, UnsignedTransaction::TimestampComparator> UnsignedTransactionSet;

  static inline UnsignedTransactionSet&
  operator<<(UnsignedTransactionSet& set, const UnsignedTransactionPtr& val){
    if(!set.insert(val).second)
      LOG(ERROR) << "cannot insert " << val->ToString() << " into UnsignedTransactionSet.";
    return set;
  }

  static inline UnsignedTransactionSet&
  operator<<(UnsignedTransactionSet& set, const UnsignedTransaction& val){
    return set << UnsignedTransaction::CopyFrom(val);
  }

  namespace json{
    static inline bool
    Write(Writer& writer, const UnsignedTransaction& val){//TODO: better error handling
      JSON_START_OBJECT(writer);
      {
        if(!json::SetField(writer, "hash", val.hash()))
          return false;
        if(!json::SetField(writer, "timestamp", val.timestamp()))
          return false;
        //TODO: serialize inputs
        //TODO: serialize outputs
      }
      JSON_END_OBJECT(writer);
      return true;
    }

    static inline bool
    Write(Writer& writer, const UnsignedTransactionPtr& val){
      return Write(writer, *val);
    }

    static inline bool
    SetField(Writer& writer, const char* name, const UnsignedTransaction& val){
      JSON_KEY(writer, name);
      return Write(writer, val);
    }

    static inline bool
    SetField(Writer& writer, const char* name, const UnsignedTransactionPtr& val){
      return SetField(writer, name, *val);
    }
  }
}
#endif//TOKEN_UNSIGNED_TRANSACTION_H