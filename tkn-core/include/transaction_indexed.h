#ifndef TOKEN_INDEXED_TRANSACTION_H
#define TOKEN_INDEXED_TRANSACTION_H

#include "json.h"
#include "transaction.h"

namespace token{
class IndexedTransaction : public internal::TransactionBase {
 public:
  struct IndexComparator {
    bool operator()(const IndexedTransactionPtr &a, const IndexedTransactionPtr &b) {
      return a->index() < b->index();
    }
  };

  class Encoder : public codec::TransactionEncoder<IndexedTransaction> {
  public:
    explicit Encoder(IndexedTransaction* value, const codec::EncoderFlags &flags = codec::kDefaultEncoderFlags) :
      TransactionEncoder<IndexedTransaction>(value, flags) {}
    Encoder(const Encoder &other) = default;
    ~Encoder() override = default;
    int64_t GetBufferSize() const override;
    bool Encode(const BufferPtr &buff) const override;
    Encoder &operator=(const Encoder &other) = default;
  };

  class Decoder : public codec::TransactionDecoder<IndexedTransaction> {
  public:
    explicit Decoder(const codec::DecoderHints& hints):
      TransactionDecoder<IndexedTransaction>(hints){}
    Decoder(const Decoder& other) = default;
    ~Decoder() override = default;
    bool Decode(const BufferPtr &buff, IndexedTransaction &result) const override;
    Decoder& operator=(const Decoder& other) = default;
  };
 private:
  int64_t index_;
 public:
  IndexedTransaction():
    internal::TransactionBase(),
    index_(){}
  IndexedTransaction(const int64_t &index,
                     const Timestamp &timestamp,
                     const InputList &inputs,
                     const OutputList &outputs) :
    internal::TransactionBase(timestamp, inputs, outputs),
    index_(index) {}
  IndexedTransaction(const IndexedTransaction &other) = default;
  ~IndexedTransaction() override = default;

  Type type() const override {
    return Type::kIndexedTransaction;
  }

  int64_t &index() {
    return index_;
  }

  int64_t index() const {
    return index_;
  }

  Hash hash() const override{
    return Hash();
  }

  std::string ToString() const override;

  IndexedTransaction &operator=(const IndexedTransaction &other) = default;

  static inline IndexedTransactionPtr
  NewInstance(const int64_t& index, const Timestamp& timestamp, const InputList& inputs, const OutputList& outputs){
    return std::make_shared<IndexedTransaction>(index, timestamp, inputs, outputs);
  }

  static inline bool
  Decode(const BufferPtr& buff, IndexedTransaction& result, const codec::DecoderHints& hints=codec::kDefaultDecoderHints){
    Decoder decoder(hints);
    return decoder.Decode(buff, result);
  }

  static inline IndexedTransactionPtr
  DecodeNew(const BufferPtr& buff, const codec::DecoderHints& hints=codec::kDefaultDecoderHints){
    IndexedTransaction instance;
    if(!Decode(buff, instance, hints))
      return nullptr;
    return std::make_shared<IndexedTransaction>(instance);
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

    if(!json::SetField(writer, "inputs", val->inputs())){
      LOG(WARNING) << "cannot set inputs field for json object.";
      return false;
    }

    if(!json::SetField(writer, "outputs", val->outputs())){
      LOG(WARNING) << "cannot set outputs field for json object.";
      return false;
    }

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