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
#include "unsigned_transaction.h"
#include "unclaimed_transaction.h"

#include "relaxed_atomic.h"

#define FOR_EACH_POOL_TYPE(V) \
  V(Block)                    \
  V(UnsignedTransaction)      \
  V(UnclaimedTransaction)

namespace token{
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

  class ObjectPool;
  typedef std::shared_ptr<ObjectPool> ObjectPoolPtr;

  static inline std::string
  GetObjectPoolDirectory(){
    std::stringstream ss;
    ss << TOKEN_BLOCKCHAIN_HOME << "/pool";
    return ss.str();
  }

#define LOG_POOL(LevelName) \
  LOG(LevelName) << "[pool] "
#define DLOG_POOL(LevelName) \
  DLOG(LevelName) << "[pool] "
#define DLOG_POOL_IF(LevelName, Condition) \
  DLOG_IF(LevelName, Condition) << "[pool] "

  class ObjectPool{
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
   protected:
    RelaxedAtomic<State> state_;
    leveldb::DB* index_;

    void SetState(const State& state){
      state_ = state;
    }

    inline leveldb::DB*
    GetIndex() const{
      return index_;
    }

    leveldb::Status InitializeIndex(const std::string& filename);
   public:
    ObjectPool():
      state_(State::kUninitialized),
      index_(nullptr){}
    virtual ~ObjectPool() = default;

    /**
     * Returns the State of the ObjectPool.
     * @see State
     * @return The State of the ObjectPool
     */
    State GetState() const{
      return (State)state_;
    }

    /**
     * Creates the following pool functions for each pool type:
     *  - WaitFor<Type>(const Hash& hash, const int64_t timeout_ms) *Deprecated*
     *  - Put<Type>(const Hash& hash, const <Type>Ptr& val);
     *  - Get<Type>(Json::Writer& writer);
     *  - Has<Type>(const Hash& hash);
     *  - Remove<Type>(const Hash& hash);
     *  - Visit<Type>s(ObjectPoolVisitor* vis);
     *  - <Type>Ptr Get<Type>(const Hash& hash);
     *  - int64_t GetNumberOf<Type>s();
     */
#define DEFINE_TYPE_METHODS(Name) \
    bool Print##Name##s(const google::LogSeverity& severity=google::INFO) const; \
    virtual bool Put##Name(const Hash& hash, const Name##Ptr& val) const;               \
    virtual bool Get##Name##s(json::Writer& json) const;                                \
    virtual bool Get##Name##s(HashList& hashes) const;                                  \
    virtual bool Has##Name(const Hash& hash) const;                                     \
    virtual bool Has##Name##s() const;    \
    virtual bool Remove##Name(const Hash& hash) const;                                  \
    virtual bool Visit##Name##s(ObjectPool##Name##Visitor* vis) const;                  \
    virtual Name##Ptr Get##Name(const Hash& hash) const;                                \
    virtual int64_t GetNumberOf##Name##s() const;
    FOR_EACH_POOL_TYPE(DEFINE_TYPE_METHODS)
#undef DEFINE_TYPE_METHODS

    //TODO: refactor
    int64_t GetNumberOfObjects() const;
    UnclaimedTransactionPtr FindUnclaimedTransaction(const Input& input) const;
    leveldb::Status Write(const leveldb::WriteBatch& update) const;

#define DEFINE_CHECK(Name) \
    inline bool Is##Name() const{ return GetState() == ObjectPool::k##Name; }
    FOR_EACH_POOL_STATE(DEFINE_CHECK)
#undef DEFINE_CHECK

    static inline const char*
    GetName(){
      return "ObjectPool";
    }

    static ObjectPoolPtr GetInstance();
    static bool Initialize(const std::string& filename=GetObjectPoolDirectory());
  };
}

#endif //TOKEN_POOL_H