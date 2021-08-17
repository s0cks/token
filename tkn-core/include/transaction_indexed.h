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
  class IndexedTransaction : public internal::TransactionBase<RawIndexedTransaction>{
  public:
  struct IndexComparator {
    bool operator()(const IndexedTransactionPtr &a, const IndexedTransactionPtr &b) {
      return a->index() < b->index();
    }
  };

  class Builder : public TransactionBuilderBase<IndexedTransaction>{
  public:
    explicit Builder(RawIndexedTransaction* raw):
      TransactionBuilderBase<IndexedTransaction>(raw){}
    Builder() = default;
    Builder(const Builder& rhs) = default;
    ~Builder() override = default;

    void SetIndex(const uint64_t& val){
      raw_->set_index(val);
    }

    IndexedTransactionPtr Build() const override{
      return std::make_shared<IndexedTransaction>(*raw_);
    }

    Builder& operator=(const Builder& rhs) = default;
  };
 public:
  IndexedTransaction():
    internal::TransactionBase<RawIndexedTransaction>(){}
  explicit IndexedTransaction(RawIndexedTransaction raw):
    internal::TransactionBase<RawIndexedTransaction>(std::move(raw)){}
  explicit IndexedTransaction(const internal::BufferPtr& data):
    internal::TransactionBase<RawIndexedTransaction>(data){}
  IndexedTransaction(const IndexedTransaction &other) = default;
  ~IndexedTransaction() override = default;

  Type type() const override {
    return Type::kIndexedTransaction;
  }

  uint64_t index() const{
    return raw_.index();
  }

  uint64_t GetNumberOfInputs() const{
    return static_cast<uint64_t>(raw_.inputs_size());
  }

  uint64_t GetNumberOfOutputs() const{
    return static_cast<uint64_t>(raw_.outputs_size());
  }

  Hash hash() const override{
    auto data = ToBuffer();
    return Hash::ComputeHash<CryptoPP::SHA256>(data->data(), data->length());
  }

  TransactionReference GetReference() const{
    return TransactionReference(hash(), index());
  }

  internal::BufferPtr ToBuffer() const;
  std::string ToString() const override;

  IndexedTransaction &operator=(const IndexedTransaction &other) = default;

  friend bool operator==(const IndexedTransaction& lhs, const IndexedTransaction& rhs){
    return lhs.hash() == rhs.hash();
  }

  friend bool operator!=(const IndexedTransaction& lhs, const IndexedTransaction& rhs){
    return lhs.hash() != rhs.hash();
  }

  static inline IndexedTransactionPtr
  NewInstance(const Timestamp& timestamp, const uint64_t& index){//TODO: use input list & output list
    return nullptr;
  }

  static inline IndexedTransactionPtr
  From(const RawIndexedTransaction& val){
    return std::make_shared<IndexedTransaction>(val);
  }

  static inline IndexedTransactionPtr
  From(const internal::BufferPtr& val){
    return std::make_shared<IndexedTransaction>(val);
  }

  static inline IndexedTransactionPtr
  CopyFrom(const IndexedTransactionPtr& val){
    return From(val->raw_);//TODO: optimize
  }
};

namespace json{
  static inline bool
  SetField(Writer& writer, const char* name, const IndexedTransactionPtr& val){
    if(!writer.Key(name, strlen(name))){
      LOG(WARNING) << "cannot set key '" << name << "'";
      return false;
    }

    if(!writer.StartObject()){
      LOG(WARNING) << "cannot start json object.";
      return false;
    }

    if(!json::SetField(writer, "index", val->index())){
      LOG(WARNING) << "cannot set index field for json object.";
      return false;
    }

    if(!json::SetField(writer, "timestamp", val->timestamp())){
      LOG(WARNING) << "cannot set timestamp field for json object.";
      return false;
    }

//TODO:
//    if(!json::SetField(writer, "inputs", val->inputs())){
//      LOG(WARNING) << "cannot set inputs field for json object.";
//      return false;
//    }
//    if(!json::SetField(writer, "outputs", val->outputs())){
//      LOG(WARNING) << "cannot set outputs field for json object.";
//      return false;
//    }

    if(!writer.EndObject()){
      LOG(WARNING) << "cannot end json object.";
      return false;
    }
    return true;
  }
}

typedef std::set<IndexedTransactionPtr, IndexedTransaction::IndexComparator> IndexedTransactionSet;
}

#endif//TOKEN_INDEXED_TRANSACTION_H