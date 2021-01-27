#ifndef TOKEN_POOL_H
#define TOKEN_POOL_H

#include <leveldb/db.h>
#include <leveldb/slice.h>
#include <leveldb/comparator.h>
#include <leveldb/write_batch.h>
#include "hash.h"
#include "block.h"
#include "transaction.h"
#include "unclaimed_transaction.h"
#include "utils/json_conversion.h"

namespace Token{
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

#define FOR_EACH_POOL_STATUS(V) \
    V(Ok)                       \
    V(Warning)                  \
    V(Error)

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

    enum Status{
#define DEFINE_STATUS(Name) k##Name,
      FOR_EACH_POOL_STATUS(DEFINE_STATUS)
#undef DEFINE_STATUS
    };

    friend std::ostream& operator<<(std::ostream& stream, const Status& status){
      switch(status){
#define DEFINE_TOSTRING(Name) \
        case Status::k##Name: \
            return stream << #Name;
        FOR_EACH_POOL_STATUS(DEFINE_TOSTRING)
#undef DEFINE_TOSTRING
        default:
          return stream << "Unknown";
      }
    }

   private:
    ObjectPool() = delete;

    static void SetState(const State& state);
    static void SetStatus(const Status& status);
   public:
    ~ObjectPool() = delete;

    /**
     * Returns the State of the ObjectPool.
     * @see State
     * @return The State of the ObjectPool
     */
    static State GetState();

    /**
     * Returns the Status of the ObjectPool.
     * @see Status
     * @return The Status of the ObjectPool
     */
    static Status GetStatus();

    /**
     * Initializes the ObjectPool.
     * @return true if successful, false otherwise
     */
    static bool Initialize();

#define DEFINE_TYPE_METHODS(Name) \
    static bool WaitFor##Name(const Hash& hash, const int64_t timeout_ms=1000*5); \
    static bool Put##Name(const Hash& hash, const Name##Ptr& val);                \
    static bool Get##Name##s(Json::Writer& json);                                   \
    static bool Has##Name(const Hash& hash);                                      \
    static bool Remove##Name(const Hash& hash);                                   \
    static bool Visit##Name##s(ObjectPool##Name##Visitor* vis);                   \
    static Name##Ptr Get##Name(const Hash& hash);                                 \
    static int64_t GetNumberOf##Name##s();
    FOR_EACH_POOL_TYPE(DEFINE_TYPE_METHODS)
#undef DEFINE_TYPE_METHODS

    //TODO: refactor
    static int64_t GetNumberOfObjects();
    static UnclaimedTransactionPtr FindUnclaimedTransaction(const Input& input);
    static leveldb::Status Write(leveldb::WriteBatch* update);

    #ifdef TOKEN_DEBUG
      static bool GetStats(Json::Writer& json);
    #endif//TOKEN_DEBUG

#define DEFINE_CHECK(Name) \
    static inline bool Is##Name(){ return GetState() == ObjectPool::k##Name; }
    FOR_EACH_POOL_STATE(DEFINE_CHECK)
#undef DEFINE_CHECK

#define DEFINE_CHECK(Name) \
    static inline bool Is##Name(){ return GetStatus() == ObjectPool::k##Name; }
    FOR_EACH_POOL_STATUS(DEFINE_CHECK)
#undef DEFINE_CHECK
  };
}

#endif //TOKEN_POOL_H