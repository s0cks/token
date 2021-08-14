#ifndef TKN_BATCH_H
#define TKN_BATCH_H

#include <mutex>
#include <leveldb/write_batch.h>

#include "pool.h"
#include "atomic/relaxed_atomic.h"

namespace token{
  namespace internal{
    class WriteBatch{
    protected:
      WriteBatch* parent_;
      leveldb::WriteBatch batch_;
      std::mutex mutex_;
      atomic::RelaxedAtomic<uint64_t> size_;
    public:
      explicit WriteBatch(WriteBatch* parent):
        parent_(parent),
        batch_(),
        mutex_(),
        size_(0){}
      WriteBatch():
        WriteBatch(nullptr){}
      ~WriteBatch() = default;

      WriteBatch* GetParent() const{
        return parent_;
      }

      leveldb::WriteBatch* batch(){
        return &batch_;
      }

      bool HasParent() const{
        return parent_ != nullptr;
      }

      uint64_t GetExpectedSize() const{
        return (uint64_t)size_;
      }

      void Append(const leveldb::WriteBatch& batch){
        DLOG(INFO) << "appending batch of size: " << batch.ApproximateSize() << "b.....";
        size_ += batch.ApproximateSize();

        std::lock_guard<std::mutex> guard(mutex_);
        batch_.Append(batch);
      }

      bool AppendToParent(){
        if(!HasParent())
          return false;
        std::lock_guard<std::mutex> guard(mutex_);
        GetParent()->Append(batch_);
        return true;
      }
    };

    class PoolWriteBatch : public WriteBatch{
    public:
      explicit PoolWriteBatch(WriteBatch* parent):
        WriteBatch(parent){}
      PoolWriteBatch() = default;
      ~PoolWriteBatch() = default;

      void PutUnclaimedTransaction(const Hash& k, const std::shared_ptr<UnclaimedTransaction>& val){
        return ObjectPool::PutUnclaimedTransactionObject(&batch_, k, val);
      }
    };
  }
}

#endif//TKN_BATCH_H