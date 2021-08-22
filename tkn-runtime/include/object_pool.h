#ifndef TKN_OBJECT_POOL_H
#define TKN_OBJECT_POOL_H

#include <memory>
#include <string>
#include <iostream>
#include <leveldb/db.h>
#include <glog/logging.h>

#include "batch.h"
#include "block.h"
#include "transaction_unsigned.h"
#include "transaction_unclaimed.h"
#include "atomic/relaxed_atomic.h"

namespace token{
  namespace internal{
#define FOR_EACH_OBJECT_POOL_STATE(V) \
  V(Uninitialized)                    \
  V(Initializing)                     \
  V(Initialized)

    class PoolBase{
    public:
      enum State{
#define DEFINE_STATE(Name) k##Name,
        FOR_EACH_OBJECT_POOL_STATE(DEFINE_STATE)
#undef DEFINE_STATE
      };

      inline friend std::ostream&
      operator<<(std::ostream& stream, const State& state){
        switch(state){
#define DEFINE_TOSTRING(Name) \
        case State::k##Name:  \
          return stream << #Name;
          FOR_EACH_OBJECT_POOL_STATE(DEFINE_TOSTRING)
#undef DEFINE_TOSTRING
          default:
            return stream << "Unknown";
        }
      }
    protected:
      atomic::RelaxedAtomic<State> state_;
      std::string filename_;
      leveldb::DB* index_;

      inline leveldb::DB*
      index() const{
        return index_;
      }

      inline leveldb::Status
      PutObject(const Hash& hash, const BufferPtr& data) const{
        leveldb::WriteOptions options;
        SetPoolWriteOptions(options);
        return index()->Put(options, (const leveldb::Slice)hash, data->AsSlice());
      }

      inline leveldb::Status
      DeleteObject(const Hash& hash) const{
        leveldb::WriteOptions options;
        SetPoolWriteOptions(options);
        return index()->Delete(options, (const leveldb::Slice)hash);
      }

      inline leveldb::Status
      GetObject(const Hash& hash, std::string& data) const{
        leveldb::ReadOptions options;
        SetPoolReadOptions(options);
        return index()->Get(options, (const leveldb::Slice)hash, &data);
      }

      leveldb::Status InitializeIndex(){
        if(IsInitialized())
          return leveldb::Status::NotSupported("cannot re-initialize the pool.");

        DLOG(INFO) << "initializing pool in " << filename_ << ".....";
        SetState(State::kInitializing);

        leveldb::Options options;
        //TODO: set comparator
        options.create_if_missing = true;

        leveldb::Status status;
        if(!(status = leveldb::DB::Open(options, filename_, &index_)).ok())
          return status;

        SetState(State::kInitialized);
        DLOG(INFO) << "initialized pool.";
        return leveldb::Status::OK();
      }

      void SetState(const State& state){
        state_ = state;
      }

      static inline void
      SetPoolReadOptions(leveldb::ReadOptions& options){
        //TODO: implement?
      }

      static inline void
      SetPoolWriteOptions(leveldb::WriteOptions& options){
        options.sync = true;
      }
    public:
      explicit PoolBase(const std::string& filename):
        state_(State::kUninitialized),
        filename_(filename),
        index_(nullptr){
        leveldb::Status status;
        if(!(status = InitializeIndex()).ok()){
          DLOG(FATAL) << "cannot initialize pool: " << status.ToString();
          return;
        }
      }
      virtual ~PoolBase(){
        delete index_;
      }

      State GetState() const{
        return (State)state_;
      }

      bool Commit(internal::WriteBatch* batch) const{
        leveldb::WriteOptions options;
        SetPoolWriteOptions(options);

        leveldb::Status status;
        if(!(status = index()->Write(options, &batch->raw())).ok()){
          LOG(ERROR) << "cannot commit batch of size " << batch->ToString();
          return false;
        }
        DLOG(INFO) << "committed batch of size " << batch->ToString();
        return true;
      }

#define DEFINE_STATE_CHECK(Name) \
    inline bool Is##Name() const{ return GetState() == State::k##Name; }
      FOR_EACH_OBJECT_POOL_STATE(DEFINE_STATE_CHECK)
#undef DEFINE_STATE_CHECK
    };

    template<class T>
    class ObjectPool : public PoolBase{
    protected:
      Type type_;

      class Iterator{
      protected:
        leveldb::Iterator* iter_;
      public:
        explicit Iterator(leveldb::DB* index, const leveldb::ReadOptions& options=leveldb::ReadOptions()):
          iter_(index->NewIterator(options)){
          iter_->SeekToFirst();
        }
        ~Iterator(){
          delete iter_;
        }

        bool HasNext() const{
          return iter_ && iter_->Valid();
        }

        std::shared_ptr<T> Next() const{
          auto data = internal::CopyBufferFrom(iter_->value());
          auto next = T::From(data);
          iter_->Next();
          return next;
        }
      };

      ObjectPool(const Type& type, const std::string& filename):
        PoolBase(filename),
        type_(type){}
    public:
      ~ObjectPool() override = default;

      Type GetPoolType() const{
        return type_;
      }

      virtual std::shared_ptr<T> Get(const Hash& hash) const{
        std::string data;
        leveldb::Status status;
        if(!(status = GetObject(hash, data)).ok()){
          if(status.IsNotFound()){
            DVLOG(2) << "cannot find " << hash << " (" << GetPoolType() << ") in pool.";
            return nullptr;
          }
          LOG(ERROR) << "cannot find " << hash << " (" << GetPoolType() << ") in pool: " << status.ToString();
          return nullptr;
        }
        DVLOG(2) << "found " << hash << " (" << GetPoolType() << ").";
        return T::From(internal::CopyBufferFrom(data));
      }

      virtual bool Has(const Hash& hash) const{
        std::string data;
        leveldb::Status status;
        if(!(status = GetObject(hash, data)).ok()){
          if(status.IsNotFound()){
            DVLOG(2) << "cannot find " << hash << " (" << GetPoolType() << ") in pool.";
            return false;
          }
          LOG(ERROR) << "cannot find " << hash << " (" << GetPoolType() << ") in pool: " << status.ToString();
          return false;
        }
        DVLOG(2) << "found " << hash << " (" << GetPoolType() << ").";
        return true;
      }

      virtual bool Put(const Hash& hash, const std::shared_ptr<T>& val) const{
        auto data = val->ToBuffer();
        leveldb::Status status;
        if(!(status = PutObject(hash, data)).ok()){
          LOG(ERROR) << "cannot index " << hash << " (" << GetPoolType() << ") in pool: " << status.ToString();
          return false;
        }
        DVLOG(2) << "indexed " << hash << " (" << GetPoolType() << ") in pool.";
        return true;
      }

      virtual bool Delete(const Hash& hash) const{
        leveldb::Status status;
        if(!(status = DeleteObject(hash)).ok()){
          LOG(ERROR) << "cannot delete " << hash << " (" << GetPoolType() << ") in pool: " << status.ToString();
          return false;
        }
        DVLOG(2) << "deleted " << hash << " (" << GetPoolType() << ") in pool.";
        return true;
      }

      virtual void GetAll(HashList& hashes) const{
        Iterator iterator(index());
        while(iterator.HasNext()){
          auto next = iterator.Next();
          DLOG(INFO) << "next: " << next->hash();
          hashes.push_back(next->hash());
        }//TODO: optimize?
      }

      virtual uint64_t GetNumberInPool() const{
        uint64_t count = 0;
        Iterator iterator(index());
        while(iterator.HasNext()){
          count++;
          iterator.Next();
        }
        return count;//TODO: optimize
      }
    };
  }

  class UnsignedTransactionPool : public internal::ObjectPool<UnsignedTransaction>{
  public:
    explicit UnsignedTransactionPool(const std::string& filename):
      internal::ObjectPool<UnsignedTransaction>(Type::kUnsignedTransaction, filename + "/unsigned_transactions"){}
    ~UnsignedTransactionPool() override = default;
    bool Visit(UnsignedTransactionVisitor* vis) const;
  };

  class BlockPool : public internal::ObjectPool<Block>{
  public:
    explicit BlockPool(const std::string& filename):
      internal::ObjectPool<Block>(Type::kBlock, filename + "/blocks"){}
    ~BlockPool() override = default;
    bool Visit(BlockVisitor* vis) const;
  };

  class UnclaimedTransactionPool : public internal::ObjectPool<UnclaimedTransaction>{
  public:
    explicit UnclaimedTransactionPool(const std::string& filename):
      internal::ObjectPool<UnclaimedTransaction>(Type::kUnclaimedTransaction, filename + "/unclaimed_transactions"){}
    ~UnclaimedTransactionPool() override = default;
    bool Visit(UnclaimedTransactionVisitor* vis) const;
  };
}

#endif//TKN_OBJECT_POOL_H