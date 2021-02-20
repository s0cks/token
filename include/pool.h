#ifndef TOKEN_POOL_H
#define TOKEN_POOL_H

#include <glog/logging.h>
#include <leveldb/db.h>
#include <leveldb/slice.h>
#include <leveldb/comparator.h>
#include <leveldb/write_batch.h>

#include "key.h"
#include "hash.h"
#include "block.h"
#include "transaction.h"
#include "unclaimed_transaction.h"

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

    class PoolKey : public KeyType{
     public:
      static inline int
      Compare(const PoolKey& a, const PoolKey& b){
        int result;
        // compare the objects type first
        if((result = ObjectTag::CompareType(a.tag(), b.tag())) != 0)
          return result; // not equal

        // if the objects are the same type & size, compare the hash.
        return Hash::Compare(a.GetHash(), b.GetHash());
      }
     private:
      enum Layout{
        kTagPosition = 0,
        kBytesForTag = sizeof(RawObjectTag),

        kHashPosition = kTagPosition+kBytesForTag,
        kBytesForHash = Hash::kSize,

        kTotalSize = kHashPosition+kBytesForHash,
      };

      uint8_t data_[kTotalSize];

      inline RawObjectTag*
      tag_ptr() const{
        return (RawObjectTag*)&data_[kTagPosition];
      }

      inline void
      SetTag(const RawObjectTag& tag){
        memcpy(&data_[kTagPosition], &tag, kBytesForTag);
      }

      inline void
      SetTag(const ObjectTag& tag){
        return SetTag(tag.raw());
      }

      inline uint8_t*
      hash_ptr() const{
        return (uint8_t*)&data_[kHashPosition];
      }

      inline void
      SetHash(const Hash& hash){
        memcpy(&data_[kHashPosition], hash.data(), kBytesForHash);
      }
     public:
      PoolKey(const ObjectTag& tag, const Hash& hash):
          KeyType(),
          data_(){
        SetTag(tag);
        SetHash(hash);
      }
      PoolKey(const Type& type, const int16_t& size, const Hash& hash):
          PoolKey(ObjectTag(type, size), hash){}
      PoolKey(const leveldb::Slice& slice):
          KeyType(),
          data_(){
        memcpy(data_, slice.data(), std::min(slice.size(), (size_t)kTotalSize));
      }
      ~PoolKey() = default;

      ObjectTag tag() const{
        return ObjectTag(*tag_ptr());
      }

      Hash GetHash() const{
        return Hash(hash_ptr(), kBytesForHash);
      }

      size_t size() const{
        return kTotalSize;
      }

      char* data() const{
        return (char*)data_;
      }

      std::string ToString() const{
        std::stringstream ss;
        ss << "PoolKey(";
        ss << "tag=" << tag() << ", ";
        ss << "hash=" << GetHash();
        ss << ")";
        return ss.str();
      }

      friend bool operator==(const PoolKey& a, const PoolKey& b){
        return PoolKey::Compare(a, b) == 0;
      }

      friend bool operator!=(const PoolKey& a, const PoolKey& b){
        return PoolKey::Compare(a, b) != 0;
      }

      friend bool operator<(const PoolKey& a, const PoolKey& b){
        return PoolKey::Compare(a, b) < 0;
      }

      friend bool operator>(const PoolKey& a, const PoolKey& b){
        return PoolKey::Compare(a, b) > 0;
      }
    };

    class Comparator : public leveldb::Comparator{
     public:
      Comparator() = default;
      ~Comparator() = default;

      int Compare(const leveldb::Slice& a, const leveldb::Slice& b) const{
        PoolKey k1(a);
        if(!k1.valid())
          LOG(WARNING) << "k1 doesn't have a IsValid tag";

        PoolKey k2(b);
        if(!k2.valid())
          LOG(WARNING) << "k2 doesn't have a IsValid tag.";
        return PoolKey::Compare(k1, k2);
      }

      const char* Name() const{
        return "ObjectPoolComparator";
      }

      void FindShortestSeparator(std::string* str, const leveldb::Slice& slice) const{}
      void FindShortSuccessor(std::string* str) const {}
    };
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
    static bool Print##Name##s(const google::LogSeverity severity=google::INFO); \
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
    static leveldb::Status Write(const leveldb::WriteBatch& update);

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