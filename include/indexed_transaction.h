#ifndef TOKEN_INDEXED_TRANSACTION_H
#define TOKEN_INDEXED_TRANSACTION_H

#include "codec.h"
#include "encoder.h"
#include "decoder.h"
#include "timestamp.h"
#include "binary_object.h"
#include "transaction_input.h"
#include "transaction_output.h"

namespace token{
  //TODO: rename?
  class IndexedTransaction : public BinaryObject, public std::enable_shared_from_this<IndexedTransaction>{
   public:
    struct HashEqualsComparator{
      bool operator()(const IndexedTransactionPtr& a, const IndexedTransactionPtr& b){
        return a->hash() == b->hash();
      }
    };

    struct IndexComparator{
      bool operator()(const IndexedTransactionPtr& a, const IndexedTransactionPtr& b){
        return a->GetIndex() < b->GetIndex();
      }
    };

   class Encoder : public codec::EncoderBase<IndexedTransaction>{
    public:
      explicit Encoder(const IndexedTransaction& value, const codec::EncoderFlags& flags=codec::kDefaultEncoderFlags):
        codec::EncoderBase<IndexedTransaction>(value, flags){}
      Encoder(const Encoder& other) = default;
      ~Encoder() override = default;

      int64_t GetBufferSize() const override;
      bool Encode(const BufferPtr& buff) const override;
      Encoder& operator=(const Encoder& other) = default;
   };

   class Decoder : public codec::DecoderBase<IndexedTransaction>{
    public:
     Decoder(const codec::DecoderHints& hints=codec::kDefaultDecoderHints):
      codec::DecoderBase<IndexedTransaction>(hints){}
     Decoder(const Decoder& other) = default;
     ~Decoder() override = default;
     bool Decode(const BufferPtr& buff, IndexedTransaction& result) const override;
     Decoder& operator=(const Decoder& other) = default;
   };
   private:
    Timestamp timestamp_;
    InputList inputs_;
    OutputList outputs_;
    int64_t index_;
   public:
    IndexedTransaction() = default;
    IndexedTransaction(const Timestamp& timestamp, const int64_t& index, const InputList& inputs, const OutputList& outputs):
      BinaryObject(),
      timestamp_(timestamp),
      inputs_(inputs),
      outputs_(outputs),
      index_(index){}
    IndexedTransaction(const IndexedTransaction& other) = default;
    ~IndexedTransaction() override = default;

    Type type() const override{
      return Type::kIndexedTransaction;
    }

    Timestamp& timestamp(){
      return timestamp_;
    }

    Timestamp timestamp() const{
      return timestamp_;
    }

    int64_t GetIndex() const{
      return index_;
    }

    InputList& inputs(){
      return inputs_;
    }

    InputList inputs() const{
      return inputs_;
    }

    OutputList& outputs(){
      return outputs_;
    }

    OutputList outputs() const{
      return outputs_;
    }

    int64_t GetBufferSize() const{
      Encoder encoder((*this));
      return encoder.GetBufferSize();
    }

    BufferPtr ToBuffer() const override;
    std::string ToString() const override;

    IndexedTransaction& operator=(const IndexedTransaction& other) = default;

    static inline IndexedTransactionPtr
    NewInstance(const int64_t& index, const InputList& inputs, const OutputList& outputs, const Timestamp& timestamp=Clock::now()){
      return std::make_shared<IndexedTransaction>(timestamp, index, inputs, outputs);
    }

    static inline bool
    Decode(const BufferPtr& buff, IndexedTransaction& result, const codec::DecoderHints& hints=codec::kDefaultDecoderHints){
      Decoder decoder(hints);
      return decoder.Decode(buff, result);
    }

    static inline IndexedTransactionPtr
    DecodeNew(const BufferPtr& buff, const codec::DecoderHints& hints=codec::kDefaultDecoderHints){
      IndexedTransaction result;
      if(!Decode(buff, result, hints)){
        DLOG(ERROR) << "cannot decode IndexedTransaction.";
        return nullptr;
      }
      return std::make_shared<IndexedTransaction>(result);
    }
  };

  typedef std::vector<IndexedTransactionPtr> IndexedTransactionList;
  typedef std::set<IndexedTransactionPtr, IndexedTransaction::IndexComparator> IndexedTransactionSet;
}

#endif//TOKEN_INDEXED_TRANSACTION_H