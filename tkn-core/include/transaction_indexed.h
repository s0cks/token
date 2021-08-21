#ifndef TOKEN_INDEXED_TRANSACTION_H
#define TOKEN_INDEXED_TRANSACTION_H

#include "json.h"
#include "transaction.h"

namespace token{
  class IndexedTransactionVisitor{
  protected:
    IndexedTransactionVisitor() = default;
  public:
    virtual ~IndexedTransactionVisitor() = default;
    virtual bool Visit(const IndexedTransactionPtr& val) = 0;
  };

  typedef internal::proto::IndexedTransaction RawIndexedTransaction;
  class IndexedTransaction: public internal::TransactionBase{
  public:
    static inline int
    CompareHash(const IndexedTransaction& lhs, const IndexedTransaction& rhs){
      auto lhs_hash = lhs.hash();
      auto rhs_hash = rhs.hash();
      if(lhs_hash < rhs_hash){
        return -1;
      } else if(lhs_hash > rhs_hash){
        return +1;
      }
      return 0;
    }

    struct HashComparator{
      bool operator()(const IndexedTransactionPtr& lhs, const IndexedTransactionPtr& rhs){
        return CompareHash(*lhs, *rhs) < 0;
      }
    };

    static inline int
    CompareIndex(const IndexedTransaction& lhs, const IndexedTransaction& rhs){
      if(lhs.index() < rhs.index()){
        return -1;
      } else if(lhs.index() > rhs.index()){
        return +1;
      }
      return 0;
    }

    struct IndexComparator{
      bool operator()(const IndexedTransactionPtr& lhs, const IndexedTransactionPtr& rhs){
        return CompareIndex(*lhs, *rhs) < 0;
      }
    };

    static inline int
    CompareTimestamp(const IndexedTransaction& lhs, const IndexedTransaction& rhs){
      if(lhs.timestamp() < rhs.timestamp()){
        return -1;
      } else if(lhs.timestamp() > rhs.timestamp()){
        return +1;
      }
      return 0;
    }

    struct TimestampComparator{
      bool operator()(const IndexedTransactionPtr& lhs, const IndexedTransactionPtr& rhs){
        return CompareTimestamp(*lhs, *rhs) < 0;
      }
    };
  protected:
    uint64_t index_;
  public:
    IndexedTransaction():
      internal::TransactionBase(),
      index_(0){
    }
    IndexedTransaction(const uint64_t& index, const Timestamp& timestamp, const InputList& inputs, const OutputList& outputs):
      internal::TransactionBase(timestamp, inputs, outputs),
      index_(index){
    }
    explicit IndexedTransaction(const RawIndexedTransaction& raw):
      internal::TransactionBase(raw),
      index_(raw.index()){
    }
    IndexedTransaction(const IndexedTransaction& rhs) = default;
    ~IndexedTransaction() override = default;

    Type type() const override{
      return Type::kIndexedTransaction;
    }

    uint64_t index() const{
      return index_;
    }

    Hash hash() const override{
      auto data = ToBuffer();
      return ComputeHash<CryptoPP::SHA256>(data);
    }

    std::string ToString() const override{
      std::stringstream ss;
      ss << "IndexedTransaction(";
      ss << "index=" << index() << ", ";
      ss << "timestamp=" << FormatTimestampReadable(timestamp()) << ", ";
      ss << "inputs=[]";
      ss << "outputs=[]";
      ss << ")";
      return ss.str();
    }

    internal::BufferPtr ToBuffer() const;

    IndexedTransaction& operator=(const IndexedTransaction& rhs) = default;

    IndexedTransaction& operator=(const RawIndexedTransaction& rhs){
      NOT_IMPLEMENTED(ERROR);//TODO: implement
      return *this;
    }

    friend std::ostream& operator<<(std::ostream& stream, const IndexedTransaction& rhs){
      return stream << rhs.ToString();
    }

    friend bool operator==(const IndexedTransaction& lhs, const IndexedTransaction& rhs){
      return CompareHash(lhs, rhs) == 0;
    }

    friend bool operator!=(const IndexedTransaction& lhs, const IndexedTransaction& rhs){
      return CompareHash(lhs, rhs) != 0;
    }

    friend bool operator<(const IndexedTransaction& lhs, const IndexedTransaction& rhs){
      return CompareIndex(lhs, rhs) < 0;
    }

    friend bool operator>(const IndexedTransaction& lhs, const IndexedTransaction& rhs){
      return CompareIndex(lhs, rhs) > 0;
    }

    static inline IndexedTransactionPtr
    NewInstance(const uint64_t& index, const Timestamp& timestamp, const InputList& inputs, const OutputList& outputs){
      return std::make_shared<IndexedTransaction>(index, timestamp, inputs, outputs);
    }

    static inline IndexedTransactionPtr
    From(const RawIndexedTransaction& val){
      return std::make_shared<IndexedTransaction>(val);
    }

    static inline IndexedTransactionPtr
    From(const internal::BufferPtr& val){
      auto length = val->GetUnsignedLong();
      DVLOG(2) << "decoded Transaction length: " << length;
      RawIndexedTransaction raw;
      if(!val->GetMessage(raw, length)){
        LOG(FATAL) << "cannot decode IndexedTransaction (" << PrettySize(length) << ") from buffer of size: " << val->length();
        return nullptr;
      }
      return From(raw);
    }

    static inline IndexedTransactionPtr
    CopyFrom(const IndexedTransaction& val){
      return std::make_shared<IndexedTransaction>(val);
    }

    static inline IndexedTransactionPtr
    CopyFrom(const IndexedTransactionPtr& val){
      return std::make_shared<IndexedTransaction>(*val);
    }
  };

  static inline RawIndexedTransaction&
  operator<<(RawIndexedTransaction& raw, const IndexedTransaction& val){
    raw.set_timestamp(ToUnixTimestamp(val.timestamp()));
    raw.set_index(val.index());
    for(auto iter = val.inputs_begin(); iter != val.inputs_end(); iter++)
      (*raw.add_inputs()) << (*iter);
    for(auto iter = val.outputs_begin(); iter != val.outputs_end(); iter++)
      (*raw.add_outputs()) << (*iter);
    return raw;
  }

  static inline RawIndexedTransaction&
  operator<<(RawIndexedTransaction& raw, const IndexedTransactionPtr& val){
    return raw << (*val);
  }

  typedef std::vector<IndexedTransactionPtr> IndexedTransactionList;

  static inline IndexedTransactionList&
  operator<<(IndexedTransactionList& list, const IndexedTransactionPtr& val){
    list.push_back(val);
    return list;
  }

  static inline IndexedTransactionList&
  operator<<(IndexedTransactionList& list, const IndexedTransaction& val){
    return list << IndexedTransaction::CopyFrom(val);
  }

  typedef std::set<IndexedTransactionPtr, IndexedTransaction::IndexComparator> IndexedTransactionSet;

  static inline IndexedTransactionSet&
  operator<<(IndexedTransactionSet& set, const IndexedTransactionPtr& val){
    if(!set.insert(val).second)
      LOG(ERROR) << "cannot insert " << val->hash() << " into IndexedTransactionSet.";
    return set;
  }

  static inline IndexedTransactionSet
  operator<<(IndexedTransactionSet& set, const IndexedTransaction& val){
    return set << IndexedTransaction::CopyFrom(val);
  }

  namespace json{
    static inline bool
    Write(Writer& writer, const IndexedTransactionPtr& val){//TODO: better error handling
      JSON_START_OBJECT(writer);
      {
        if(!json::SetField(writer, "hash", val->hash()))
          return false;
        if(!json::SetField(writer, "index", val->index()))
          return false;
        if(!json::SetField(writer, "timestamp", val->timestamp()))
          return false;

        JSON_KEY(writer, "inputs");
        JSON_START_ARRAY(writer);
        {
          for(auto iter = val->inputs_begin(); iter != val->inputs_end(); iter++){
            if(!json::Write(writer, **iter))
              return false;
          }
        }
        JSON_END_ARRAY(writer);

        JSON_KEY(writer, "outputs");
        JSON_START_ARRAY(writer);
        {
          for(auto iter = val->outputs_begin(); iter != val->outputs_begin(); iter++){
            if(!json::Write(writer, **iter))
              return false;
          }
        }
        JSON_END_ARRAY(writer);
      }
      JSON_END_OBJECT(writer);
      return true;
    }

    static inline bool
    SetField(Writer& writer, const char* name, const IndexedTransactionPtr& val){
      JSON_KEY(writer, name);
      return Write(writer, val);
    }
  }
}
#endif//TOKEN_INDEXED_TRANSACTION_H