#ifndef TOKEN_POOL_H
#define TOKEN_POOL_H

#include <glog/logging.h>
#include <leveldb/db.h>
#include <leveldb/slice.h>
#include <leveldb/comparator.h>
#include <leveldb/write_batch.h>

#include "key.h"
#include "hash.h"
#include "flags.h"

#include "block.h"
#include "transaction_unsigned.h"
#include "transaction_unclaimed.h"

#include "atomic/relaxed_atomic.h"

#define FOR_EACH_POOL_TYPE(V) \
  V(Block)                    \
  V(UnsignedTransaction)      \
  V(UnclaimedTransaction)

namespace token{

  struct PoolStats{
    uint64_t num_blocks;
    uint64_t num_transactions_unsigned;
    uint64_t num_transactions_unclaimed;

    PoolStats():
      num_blocks(0),
      num_transactions_unsigned(0),
      num_transactions_unclaimed(0){}
    PoolStats(const PoolStats& rhs) = default;
    ~PoolStats() = default;
    PoolStats& operator=(const PoolStats& rhs) = default;
  };

  namespace json{
    static inline
    bool Write(Writer& writer, const PoolStats& val){
      JSON_START_OBJECT(writer);
      {
        if(!SetField(writer, "blocks", val.num_blocks)){
          LOG(ERROR) << "cannot set blocks json field.";
          return false;
        }

        if(!SetField(writer, "unsigned_transactions", val.num_transactions_unsigned)){
          LOG(ERROR) << "cannot set unsigned transactions json field.";
          return false;
        }

        if(!SetField(writer, "unclaimed_transactions", val.num_transactions_unclaimed)){
          LOG(ERROR) << "cannot set unclaimed transactions json field.";
          return false;
        }
      }
      JSON_END_OBJECT(writer);
      return true;
    }

    static inline
    bool SetField(Writer& writer, const char* name, const PoolStats& val){
      JSON_KEY(writer, name);
      return Write(writer, val);
    }
  }

  class ObjectPoolVisitor{
   protected:
    ObjectPoolVisitor() = default;

   public:
    virtual ~ObjectPoolVisitor() = default;
  };

#define GENERATE_TYPE_VISITOR(Name) \
  class ObjectPool##Name##Visitor : public ObjectPoolVisitor{ \
    protected:                      \
      ObjectPool##Name##Visitor() = default;                  \
    public:                         \
      virtual ~ObjectPool##Name##Visitor() = default;         \
      virtual bool Visit(const Name##Ptr& val) = 0;           \
  };
  FOR_EACH_POOL_TYPE(GENERATE_TYPE_VISITOR)
#undef GENERATE_TYPE_VISITOR

#define FOR_EACH_POOL_STATE(V) \
    V(Uninitialized)           \
    V(Initializing)            \
    V(Initialized)

  static inline std::string
  GetObjectPoolDirectory(){
    std::stringstream ss;
    ss << TOKEN_BLOCKCHAIN_HOME << "/pool";
    return ss.str();
  }

  class PoolVisitor{
  protected:
    PoolVisitor() = default;
  public:
    virtual ~PoolVisitor() = default;
    virtual bool Visit(const BlockPtr& val) const = 0;
    virtual bool Visit(const UnsignedTransactionPtr& val) const = 0;
    virtual bool Visit(const UnclaimedTransactionPtr& val) const = 0;
  };

  template<const google::LogSeverity& Severity=google::INFO>
  class PoolPrinter : public PoolVisitor{
  public:
    PoolPrinter():
      PoolVisitor(){}
    ~PoolPrinter() override = default;

    bool Visit(const BlockPtr& val) const override{
      LOG_AT_LEVEL(Severity) << val->hash() << " (Block)";
      return true;
    }

    bool Visit(const UnsignedTransactionPtr& val) const override{
      LOG_AT_LEVEL(Severity) << val->hash() << " (UnsignedTransaction)";
      return true;
    }

    bool Visit(const UnclaimedTransactionPtr& val) const override{
      LOG_AT_LEVEL(Severity) << val->hash() << " (UnclaimedTransaction)";
      return true;
    }
  };

  class ObjectPool{
  private:
    static inline leveldb::Status
    DeleteObject(leveldb::DB* storage, const Type& type, const Hash& k){
      leveldb::WriteOptions options;
      options.sync = true;

      ObjectKey key(type, k);
      return storage->Delete(options, (const leveldb::Slice&)key);
    }

    static inline void
    DeleteObject(leveldb::WriteBatch* batch, const Type& type, const Hash& k){
      ObjectKey key(type, k);
      return batch->Delete((const leveldb::Slice&)key);
    }

    template<class T>
    static inline leveldb::Status
    PutObject(leveldb::DB* storage, const Type& type, const Hash& key, const std::shared_ptr<T>& val){
      leveldb::WriteOptions options;
      options.sync = true;

      ObjectKey k(type, key);
      auto data = val->ToBuffer();
      auto slice = data->AsSlice();
      return storage->Put(options, (const leveldb::Slice&)k, slice);
    }

    template<class T>
    static inline void
    PutObject(leveldb::WriteBatch* batch, const Type& type, const Hash& k, const std::shared_ptr<T>& v){
      ObjectKey key(type, k);
      auto value = v->ToBuffer();
      return batch->Put((const leveldb::Slice&)key, value->AsSlice());
    }

    static inline leveldb::Status
    HasObject(leveldb::DB* storage, const Type& type, const Hash& k){
      leveldb::ReadOptions options;

      ObjectKey key(type, k);

      std::string data;
      return storage->Get(options, (const leveldb::Slice&)key, &data);
    }

    static inline leveldb::Status
    GetObject(leveldb::DB* storage, const Type& type, const Hash& k, std::string* result){
      leveldb::ReadOptions options;
      ObjectKey key(type, k);
      return storage->Get(options, (const leveldb::Slice&)key, result);
    }
   public:
    enum State{
#define DEFINE_STATE(Name) k##Name,
      FOR_EACH_POOL_STATE(DEFINE_STATE)
#undef DEFINE_STATE
    };

    friend std::ostream& operator<<(std::ostream& stream, const State& state){
      switch(state){
#define DEFINE_TOSTRING(Name) \
        case State::k##Name: \
            return stream << #Name;
        FOR_EACH_POOL_STATE(DEFINE_TOSTRING)
#undef DEFINE_TOSTRING
        default:
          return stream << "Unknown";
      }
    }

    class Comparator : public leveldb::Comparator{
     public:
      Comparator() = default;
      ~Comparator() override = default;

      int Compare(const leveldb::Slice& a, const leveldb::Slice& b) const override{
        ObjectKey k1(a), k2(b);
        return ObjectKey::Compare(k1, k2);
      }

      const char* Name() const override{
        return "ObjectPoolComparator";
      }

      void FindShortestSeparator(std::string* str, const leveldb::Slice& slice) const override{}
      void FindShortSuccessor(std::string* str) const override {}
    };

#define DEFINE_PUT_OBJECT(Name) \
    template<class T>           \
    static inline leveldb::Status \
    Put##Name##Object(leveldb::DB* storage, const Hash& k, const std::shared_ptr<T>& val){ \
      return PutObject(storage, Type::k##Name, k, val); \
    }                           \
    template<class T>\
    static inline void          \
    Put##Name##Object(leveldb::WriteBatch* batch, const Hash& k, const std::shared_ptr<T>& val){ \
      return PutObject(batch, Type::k##Name, k, val);                            \
    }
    FOR_EACH_POOL_TYPE(DEFINE_PUT_OBJECT)
#undef DEFINE_PUT_OBJECT

#define DEFINE_DELETE_OBJECT(Name) \
    static inline leveldb::Status \
    Delete##Name##Object(leveldb::DB* storage, const Hash& k){ \
      return DeleteObject(storage, Type::k##Name, k);          \
    }                       \
    static inline void      \
    Delete##Name##Object(leveldb::WriteBatch* batch, const Hash& k){                                                                           \
      return DeleteObject(batch, Type::k##Name, k);                                                                                            \
    }
    FOR_EACH_POOL_TYPE(DEFINE_DELETE_OBJECT)
#undef DEFINE_DELETE

#define DEFINE_HAS_OBJECT(Name) \
    static inline leveldb::Status \
    Has##Name##Object(leveldb::DB* storage, const Hash& k){ \
      return HasObject(storage, Type::k##Name, k);          \
    }
    FOR_EACH_POOL_TYPE(DEFINE_HAS_OBJECT)
#undef DEFINE_HAS_OBJECT

#define DEFINE_GET_OBJECT(Name) \
    static inline leveldb::Status \
    Get##Name##Object(leveldb::DB* storage, const Hash& k, std::string* result){ \
      return GetObject(storage, Type::k##Name, k, result);          \
    }
    FOR_EACH_POOL_TYPE(DEFINE_GET_OBJECT)
#undef DEFINE_GET_OBJECT
   protected:
    atomic::RelaxedAtomic<State> state_;
    leveldb::DB* index_;
    std::string filename_;

    std::mutex mutex_;

    void SetState(const State& state){
      state_ = state;
    }

    inline leveldb::DB*
    GetIndex() const{
      return index_;
    }

    leveldb::Status InitializeIndex(const std::string& filename);

    template<class T>
    inline std::shared_ptr<T>
    GetTypeSafely(const Type& type, const Hash& hash) const{
      std::string value;

      leveldb::Status status;
      if(!(status = GetObject(GetIndex(), type, hash, &value)).ok()){
        if(status.IsNotFound()){
          DLOG(WARNING) << "cannot find " << hash << " (" << type << "): " << status.ToString();
          return nullptr;
        }

        DLOG(FATAL) << "cannot get " << hash << " (" << type << "): " << status.ToString();
        return nullptr;
      }

      return T::Decode(internal::CopyBufferFrom(value));
    }
   public:
    explicit ObjectPool(const std::string& filename);
    virtual ~ObjectPool() = default;

    std::string GetFilename() const{
      return filename_;
    }

    State GetState() const{
      return (State)state_;
    }

#define DECLARE_GET_POOL_OBJECT(Name) \
    virtual std::shared_ptr<Name> Get##Name(const Hash& k) const;
#define DECLARE_PUT_POOL_OBJECT(Name) \
    virtual bool Put##Name(const Hash& k, const std::shared_ptr<Name>& val) const;
#define DECLARE_HAS_POOL_OBJECT(Name) \
    virtual bool Has##Name(const Hash& k) const;
#define DECLARE_REMOVE_POOL_OBJECT(Name) \
    virtual bool Remove##Name(const Hash& k) const;

#define DECLARE_POOL_TYPE_METHODS(Name) \
    DECLARE_GET_POOL_OBJECT(Name)       \
    DECLARE_PUT_POOL_OBJECT(Name)       \
    DECLARE_HAS_POOL_OBJECT(Name)       \
    DECLARE_REMOVE_POOL_OBJECT(Name)

    FOR_EACH_POOL_TYPE(DECLARE_POOL_TYPE_METHODS)
#undef DECLARE_POOL_TYPE_METHODS
#undef DECLARE_REMOTE_POOL_OBJECT
#undef DECLARE_HAS_POOL_OBJECT
#undef DECLARE_GET_POOL_OBJECT
#undef DECLARE_PUT_POOL_OBJECT

#define DEFINE_TYPE_METHODS(Name) \
    bool Print##Name##s(const google::LogSeverity& severity=google::INFO) const; \
    virtual bool Get##Name##s(json::Writer& json) const;                                \
    virtual bool Get##Name##s(HashList& hashes) const;                                  \
    virtual bool Has##Name##s() const;    \
    virtual uint64_t GetNumberOf##Name##s() const;
    FOR_EACH_POOL_TYPE(DEFINE_TYPE_METHODS)
#undef DEFINE_TYPE_METHODS

    virtual bool Accept(PoolVisitor* vis) const;

    //TODO: refactor
    uint64_t GetNumberOfObjects() const;
    UnclaimedTransactionPtr FindUnclaimedTransaction(const Input& input) const;
    leveldb::Status Commit(leveldb::WriteBatch* batch);

    virtual void GetPoolStats(PoolStats& stats) const;

#define DEFINE_CHECK(Name) \
    inline bool Is##Name() const{ return GetState() == ObjectPool::k##Name; }
    FOR_EACH_POOL_STATE(DEFINE_CHECK)
#undef DEFINE_CHECK

    static inline const char*
    GetName(){
      return "ObjectPool";
    }
  };
}

#endif //TOKEN_POOL_H