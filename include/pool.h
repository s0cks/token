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
   private:
    ObjectPool() = delete;

    /**
     * Sets the ObjectPool's state
     * @see State
     * @param state - The desired state of the ObjectPool
     */
    static void SetState(const State& state);
   public:
    ~ObjectPool() = delete;

    /**
     * Returns the State of the ObjectPool.
     * @see State
     * @return The State of the ObjectPool
     */
    static State GetState();

    /**
     * Initializes the ObjectPool.
     * @return true if successful, false otherwise
     */
    static bool Initialize();

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
  };
}

#endif //TOKEN_POOL_H