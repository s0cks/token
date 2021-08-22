#ifndef TKN_BATCH_H
#define TKN_BATCH_H

#include <mutex>
#include <glog/logging.h>
#include <leveldb/write_batch.h>

#include "hash.h"
#include "atomic/linked_list.h"
#include "atomic/relaxed_atomic.h"

namespace token{
  namespace internal{
    class WriteBatch{
    private:
      leveldb::WriteBatch batch_;
    public:
      WriteBatch():
        batch_(){
      }
      WriteBatch(const WriteBatch& rhs) = delete;
      ~WriteBatch() = default;

      leveldb::WriteBatch& raw(){//TODO: investigate need?
        return batch_;
      }

      uint64_t size() const{
        return batch_.ApproximateSize();
      }

      template<class T>
      void Put(const Hash& key, const std::shared_ptr<T>& val){
        auto data = val->ToBuffer();
        return batch_.Put((const leveldb::Slice)key, data->AsSlice());
      }

      void Delete(const Hash& key){
        return batch_.Delete((const leveldb::Slice)key);
      }

      void Append(const WriteBatch& batch){
        batch_.Append(batch.batch_);
      }

      std::string ToString() const{
        std::stringstream ss;
        ss << "WriteBatch(";
        ss << "size=" << size();
        ss << ")";
        return ss.str();
      }

      WriteBatch& operator=(const WriteBatch& rhs) = delete;

      friend std::ostream&
      operator<<(std::ostream& stream, const WriteBatch& batch){
        return stream << batch.ToString();
      }
    };

    class WriteBatchList{
    private:
      atomic::LinkedList<WriteBatch> batches_;
    public:
      WriteBatchList():
        batches_(){
      }
      WriteBatchList(const WriteBatchList& rhs) = delete;
      ~WriteBatchList() = default;

      std::shared_ptr<WriteBatch> CreateNewBatch(){
        auto batch = std::make_shared<WriteBatch>();
        batches_.push_front(batch);
        return batch;
      }

      uint64_t GetNumberOfBatches() const{
        return batches_.size();
      }

      bool Compile(WriteBatch& batch){
        DLOG(INFO) << "compiling " << GetNumberOfBatches() << " batches....";
        while(!batches_.empty()){
          auto next = batches_.pop_front();
          DLOG(INFO) << "appending batch of size " << next->size();
          batch.Append(*next);
        }
        return true;
      }

      WriteBatchList& operator=(const WriteBatchList& rhs) = delete;
    };
  }
}

#endif//TKN_BATCH_H