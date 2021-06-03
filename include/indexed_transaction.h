#ifndef TOKEN_INDEXED_TRANSACTION_H
#define TOKEN_INDEXED_TRANSACTION_H

#include "transaction.h"

namespace token{
  //TODO: rename?
  class IndexedTransaction : public Transaction{
   public:
    struct HashEqualsComparator{
      bool operator()(const IndexedTransactionPtr& a, const IndexedTransactionPtr& b){
        return a->GetHash() == b->GetHash();
      }
    };

    struct IndexComparator{
      bool operator()(const IndexedTransactionPtr& a, const IndexedTransactionPtr& b){
        return a->GetIndex() < b->GetIndex();
      }
    };
   private:
    int64_t index_;
   public:
    IndexedTransaction(const Timestamp& timestamp, const int64_t& index, const InputList& inputs, const OutputList& outputs):
        Transaction(timestamp, inputs, outputs),
        index_(index){}
    ~IndexedTransaction() override = default;

    Type GetType() const override{
      return Type::kIndexedTransaction;
    }

    int64_t GetIndex() const{
      return index_;
    }

    bool Write(const BufferPtr& buffer) const override{
      return buffer->PutLong(index_)
          && Transaction::Write(buffer);
    }

    int64_t GetBufferSize() const override{
      int64_t size = 0;
      size += sizeof(int64_t); // index_
      size += Transaction::GetBufferSize();
      return size;
    }

    std::string ToString() const override{
      std::stringstream ss;
      ss << "IndexedTransaction(";
      ss << "index=" << index_ << ", ";
      ss << "timestamp=" << FormatTimestampReadable(timestamp_) << ", ";
      ss << "inputs=[]" << ", ";
      ss << "outputs=[]";
      ss << ")";
      return ss.str();
    }

    static inline IndexedTransactionPtr
    NewInstance(const int64_t& index, const InputList& inputs, const OutputList& outputs, const Timestamp& timestamp=Clock::now()){
      return std::make_shared<IndexedTransaction>(timestamp, index, inputs, outputs);
    }

    static IndexedTransactionPtr FromBytes(const BufferPtr& buff);
  };

  typedef std::vector<IndexedTransactionPtr> IndexedTransactionList;
  typedef std::set<IndexedTransactionPtr, IndexedTransaction::IndexComparator> IndexedTransactionSet;
}

#endif//TOKEN_INDEXED_TRANSACTION_H