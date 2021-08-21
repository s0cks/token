#ifndef TOKEN_SIGNED_TRANSACTION_H
#define TOKEN_SIGNED_TRANSACTION_H

#include <utility>

#include "transaction.h"

namespace token{
  class SignedTransactionVisitor{
  protected:
    SignedTransactionVisitor() = default;
  public:
    virtual ~SignedTransactionVisitor() = default;
    virtual bool Visit(const SignedTransactionPtr& val) = 0;
  };

  typedef internal::proto::SignedTransaction RawSignedTransaction;
  class SignedTransaction : public internal::TransactionBase{
  public:
    static inline int
    CompareHash(const SignedTransaction& lhs, const SignedTransaction& rhs){
      return Hash::Compare(lhs.hash(), rhs.hash());
    }

    struct HashComparator{
      bool operator()(const SignedTransactionPtr& lhs, const SignedTransactionPtr& rhs){
        return CompareHash(*lhs, *rhs) < 0;
      }
    };

    static inline int
    CompareTimestamp(const SignedTransaction& lhs, const SignedTransaction& rhs){
      if(lhs.timestamp() < rhs.timestamp()){
        return -1;
      } else if(lhs.timestamp() > rhs.timestamp()){
        return +1;
      }
      return 0;
    }

    struct TimestampComparator{
      bool operator()(const SignedTransactionPtr& lhs, const SignedTransactionPtr& rhs){
        return CompareTimestamp(*lhs, *rhs) < 0;
      }
    };
   public:
    SignedTransaction() = default;
    SignedTransaction(const Timestamp& timestamp, const InputList& inputs, const OutputList& outputs):
      internal::TransactionBase(timestamp, inputs, outputs){
    }
    explicit SignedTransaction(const RawSignedTransaction& raw):
      internal::TransactionBase(raw){}
    SignedTransaction(const SignedTransaction& lhs) = default;
    ~SignedTransaction() override = default;

    Type type() const override{
      return Type::kSignedTransaction;
    }

    Hash hash() const override{
      auto data = ToBuffer();
      return ComputeHash<CryptoPP::SHA256>(data);
    }

    std::string ToString() const override{
      NOT_IMPLEMENTED(ERROR);//TODO: implement
      return "SignedTransaction()";
    }

    internal::BufferPtr ToBuffer() const;

    SignedTransaction& operator=(const SignedTransaction& rhs) = default;

    SignedTransaction& operator=(const RawSignedTransaction& rhs){
      NOT_IMPLEMENTED(ERROR);//TODO: implement
      return *this;
    }

    friend std::ostream& operator<<(std::ostream& stream, const SignedTransaction& rhs){
      return stream << rhs.ToString();
    }

    friend bool operator==(const SignedTransaction& lhs, const SignedTransaction& rhs){
      return CompareHash(lhs, rhs) == 0;
    }

    friend bool operator!=(const SignedTransaction& lhs, const SignedTransaction& rhs){
      return CompareHash(lhs, rhs) != 0;
    }

    friend bool operator<(const SignedTransaction& lhs, const SignedTransaction& rhs){
      return CompareTimestamp(lhs, rhs) < 0;
    }

    friend bool operator>(const SignedTransaction& lhs, const SignedTransaction& rhs){
      return CompareTimestamp(lhs, rhs) > 0;
    }

    static inline SignedTransactionPtr
    NewInstance(const Timestamp& timestamp, const InputList& inputs, const OutputList& outputs){
      return std::make_shared<SignedTransaction>(timestamp, inputs, outputs);
    }

    static inline SignedTransactionPtr
    From(const RawSignedTransaction& raw){
      return std::make_shared<SignedTransaction>(raw);
    }

    static inline SignedTransactionPtr
    From(const internal::BufferPtr& val){
      auto length = val->GetUnsignedLong();
      RawSignedTransaction raw;
      if(!val->GetMessage(raw, length)){
        LOG(FATAL) << "cannot decode SignedTransaction (" << PrettySize(length) << ") from " << val->ToString();
        return nullptr;
      }
      return From(raw);
    }

    static inline SignedTransactionPtr
    CopyFrom(const SignedTransaction& val){
      return std::make_shared<SignedTransaction>(val);
    }

    static inline SignedTransactionPtr
    CopyFrom(const SignedTransactionPtr& val){
      return CopyFrom(*val);
    }
  };

  static inline RawSignedTransaction&
  operator<<(RawSignedTransaction& raw, const SignedTransaction& val){
    raw.set_timestamp(ToUnixTimestamp(val.timestamp()));
    for(auto iter = val.inputs_begin(); iter != val.inputs_end(); iter++)
      (*raw.add_inputs()) << (*iter);
    for(auto iter = val.outputs_begin(); iter != val.outputs_end(); iter++)
      (*raw.add_outputs()) << (*iter);
    return raw;
  }

  static inline RawSignedTransaction&
  operator<<(RawSignedTransaction& raw, const SignedTransactionPtr& val){
    return raw << (*val);
  }

  typedef std::vector<SignedTransactionPtr> SignedTransactionList;

  static inline SignedTransactionList&
  operator<<(SignedTransactionList& list, const SignedTransactionPtr& val){
    list.push_back(val);
    return list;
  }

  static inline SignedTransactionList&
  operator<<(SignedTransactionList& list, const SignedTransaction& val){
    return list << SignedTransaction::CopyFrom(val);
  }

  typedef std::set<SignedTransactionPtr, SignedTransaction::TimestampComparator> SignedTransactionSet;

  static inline SignedTransactionSet&
  operator<<(SignedTransactionSet& set, const SignedTransactionPtr& val){
    if(!set.insert(val).second)
      LOG(ERROR) << "cannot insert " << val->hash() << " into SignedTransactionSet.";
    return set;
  }

  static inline SignedTransactionSet&
  operator<<(SignedTransactionSet& set, const SignedTransaction& val){
    return set << SignedTransaction::CopyFrom(val);
  }

  namespace json{
    static inline bool
    SetField(Writer& writer, const char* name, const SignedTransactionPtr& val){
      NOT_IMPLEMENTED(FATAL);
      return false;
    }
  }
}

#endif//TOKEN_SIGNED_TRANSACTION_H