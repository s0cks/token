#ifndef TOKEN_UTILS_KVSTORE_H
#define TOKEN_UTILS_KVSTORE_H

#include <leveldb/db.h>
#include <leveldb/slice.h>
#include <leveldb/comparator.h>
#include <leveldb/write_batch.h>
#include "hash.h"
#include "object.h"
#include "wallet.h"
#include "unclaimed_transaction.h"

namespace token{
  class KeyType{
   protected:
    KeyType() = default;
   public:
    virtual ~KeyType() = default;
    virtual char* data() const = 0;
    virtual size_t size() const = 0;
  };

  class UserKey : public KeyType{
   public:
    class Comparator : public leveldb::Comparator{
     public:
      int Compare(const leveldb::Slice& a, const leveldb::Slice& b) const{
        UserKey k1(a);
        UserKey k2(b);
        return User::Compare(k1.user(), k2.user());
      }

      const char* Name() const{
        return "UserKeyComparator";
      }

      void FindShortestSeparator(std::string* str, const leveldb::Slice& slice) const{}
      void FindShortSuccessor(std::string* str) const{}
    };
   private:
    const User data_;
   public:
    UserKey(const User& user):
      data_(user){}
    UserKey(const leveldb::Slice& slice):
      data_((uint8_t*) slice.data(), (int64_t) slice.size()){}
    ~UserKey() = default;

    const User& user(){
      return data_;
    }

    char* data() const{
      return data_.data();
    }

    size_t size() const{
      return data_.size();
    }
  };

  class ObjectKey : public KeyType{
   public:
    static const int16_t kMagic = 0xFAFE;

    enum Layout{
      kMagicOffset = 0,
      kBytesForMagic = sizeof(int16_t),

      kTypeOffset = kMagicOffset + kBytesForMagic,
      kBytesForType = sizeof(Type),

      kHashOffset = kTypeOffset + kBytesForType,
      kBytesForHash = Hash::kSize,

      kTotalSize = kHashOffset + kBytesForHash,
    };

    class Comparator : public leveldb::Comparator{
     public:
      int Compare(const leveldb::Slice& a, const leveldb::Slice& b) const{
        ObjectKey k1(a);
        ObjectKey k2(b);

        Type t1 = k1.GetType();
        Type t2 = k2.GetType();
        if(t1 < t2){
          return -1;
        } else if(t1 > t2){
          return +1;
        }

        if(k1.IsReference() && k2.IsReference()){
          std::string r1 = k1.GetReference();
          std::string r2 = k2.GetReference();
          if(r1 < r2){
            return -1;
          } else if(r1 > r2){
            return +1;
          }

          return 0;
        }

        Hash h1 = k1.GetHash();
        Hash h2 = k2.GetHash();
        if(h1 < h2){
          return -1;
        } else if(h1 > h2){
          return +1;
        }
        return 0;
      }

      const char* Name() const{
        return "ObjectHashKeyComparator";
      }

      void FindShortestSeparator(std::string* str, const leveldb::Slice& slice) const{}
      void FindShortSuccessor(std::string* str) const{}
    };
   private:
    uint8_t data_[kTotalSize];

    uint8_t* raw(){
      return data_;
    }
   public:
    ObjectKey(const Type& type, const std::string& value):
      data_(){
      memcpy(&data_[kMagicOffset], &kMagic, kBytesForMagic);
      memcpy(&data_[kTypeOffset], &type, kBytesForType);
      memcpy(&data_[kHashOffset], value.data(), std::min(value.length(), (size_t)kBytesForHash));
    }
    ObjectKey(const Type& type, const Hash& hash):
      data_(){
      memcpy(&data_[kMagicOffset], &kMagic, kBytesForMagic);
      memcpy(&data_[kTypeOffset], &type, kBytesForType);
      memcpy(&data_[kHashOffset], hash.data(), kBytesForHash);
    }
    ObjectKey(const leveldb::Slice& value):
      data_(){
      if(value.size() < kTotalSize){
        LOG(WARNING) << "copying partial key of size " << value.size() << "(value.size() - kTotalSize).";
      } else if(value.size() > kTotalSize){
        LOG(WARNING) << "copying partial key of size " << value.size() << "(" << (value.size() - kTotalSize)
                     << " bytes remaining).";
      }
      memcpy(data_, value.data(), std::min(value.size(), (size_t) kTotalSize));
    }
    ObjectKey(const std::string& value):
      data_(){
      if(value.size() < kTotalSize){
        LOG(WARNING) << "copying partial key of size " << value.size() << "(value.size() - kTotalSize).";
      } else if(value.size() > kTotalSize){
        LOG(WARNING) << "copying partial key of size " << value.size() << "(" << (value.size() - kTotalSize)
                     << " bytes remaining).";
      }
      memcpy(data_, value.data(), std::min(value.size(), (size_t) kTotalSize));
    }
    ~ObjectKey() = default;

    char* data() const{
      return (char*) data_;
    }

    size_t size() const{
      return kTotalSize;
    }

    Type GetType() const{
      return *((Type*) &data_[kTypeOffset]);
    }

    Hash GetHash() const{
      return Hash(&data_[kHashOffset], kBytesForHash);
    }

    std::string GetReference() const{
      return std::string((char*)&data_[kHashOffset], kBytesForHash);
    }

    int16_t GetMagic() const{
      return *((int16_t*) &data_[kMagicOffset]);
    }

    bool IsValid() const{
      return GetMagic() == kMagic;
    }

#define DEFINE_CHECK(Name) \
    bool Is##Name() const{ \
      return GetType() == Type::k##Name; \
    }
    FOR_EACH_POOL_TYPE(DEFINE_CHECK)
#undef DEFINE_CHECK

    bool IsReference() const{
      return GetType() == Type::kReferenceType;
    }

    void operator=(const ObjectKey& key){
      memcpy(data_, key.data_, kTotalSize);
    }

    friend bool operator==(const ObjectKey& a, const ObjectKey& b){
      return memcmp(a.data_, b.data_, kTotalSize) == 0;
    }

    friend bool operator!=(const ObjectKey& a, const ObjectKey& b){
      return memcmp(a.data_, b.data_, kTotalSize) != 0;
    }

    friend std::ostream& operator<<(std::ostream& stream, const ObjectKey& key){
      return stream << "Key(" << key.GetType() << "," << key.GetHash() << ")";
    }
  };

  template<class K>
  static inline leveldb::Status
  GetObject(leveldb::DB* index, const K& k, std::string* val){
    leveldb::Slice key(k.data(), k.size());
    leveldb::ReadOptions readOpts;
    return index->Get(readOpts, key, val);
  }

#define DEFINE_GETTER(Name) \
  static inline leveldb::Status \
  Get##Name##Object(leveldb::DB* index, const Hash& hash, std::string* val){ \
    return GetObject(index, ObjectKey(Type::k##Name, hash), val); \
  }
  FOR_EACH_POOL_TYPE(DEFINE_GETTER)
#undef DEFINE_GETTER

  static inline leveldb::Status
  GetWallet(leveldb::DB* index, const User& user, std::string* val){
    return GetObject(index, UserKey(user), val);
  }

  template<class K>
  static inline leveldb::Status
  PutRawObject(leveldb::DB* index, const K& k, const BufferPtr& v){
    leveldb::Slice key(k.data(), k.size());
    leveldb::Slice val(v->data(), v->GetWrittenBytes());

    leveldb::WriteOptions writeOpts;
    writeOpts.sync = true;
    return index->Put(writeOpts, key, val);
  }

  template<class K>
  static inline void
  PutRawObject(leveldb::WriteBatch& batch, const K& k, const BufferPtr& v){
    leveldb::Slice key(k.data(), k.size());
    leveldb::Slice val(v->data(), v->GetWrittenBytes());
    return batch.Put(key, val);
  }

  template<class K, class V>
  static inline leveldb::Status
  PutObject(leveldb::DB* index, const K& k, const std::shared_ptr<V>& obj){
    BufferPtr vdata = Buffer::NewInstance(obj->GetBufferSize());
    if(!obj->Write(vdata)){
      std::stringstream ss;
      ss << "Cannot serialize object to buffer of size: " << vdata->GetBufferSize();
      return leveldb::Status::InvalidArgument(ss.str());
    }
    return PutRawObject(index, k, vdata);
  }

  template<class K, class V>
  static inline void
  PutObject(leveldb::WriteBatch& batch, const K& k, const std::shared_ptr<V>& obj){
    BufferPtr vdata = Buffer::NewInstance(obj->GetBufferSize());
    if(!obj->Write(vdata)){
      std::stringstream ss;
      ss << "Cannot serialize object to buffer of size: " << vdata->GetBufferSize();
      return;
    }
    return PutRawObject(batch, k, vdata);
  }

#define DEFINE_WRITERS(Name) \
  static inline leveldb::Status \
  PutObject(leveldb::DB* index, const Hash& hash, const Name##Ptr& val){ \
    return PutObject(index, ObjectKey(Type::k##Name, hash), val); \
  }                                \
  static inline void   \
  PutObject(leveldb::WriteBatch& batch, const Hash& hash, const Name##Ptr& val){ \
    return PutObject(batch, ObjectKey(Type::k##Name, hash), val);  \
  }
  FOR_EACH_POOL_TYPE(DEFINE_WRITERS)
#undef DEFINE_WRITERS

  static inline leveldb::Status
  PutObject(leveldb::DB* index, const User& user, const Wallet& wallet){
    BufferPtr buff = Buffer::NewInstance(GetBufferSize(wallet));
    Encode(wallet, buff);
    return PutRawObject(index, UserKey(user), buff);
  }

  static inline void
  PutObject(leveldb::WriteBatch& batch, const User& user, const Wallet& wallet){
    BufferPtr buff = Buffer::NewInstance(GetBufferSize(wallet));
    Encode(wallet, buff);
    return PutRawObject(batch, UserKey(user), buff);
  }

  template<class K>
  static inline bool
  HasObject(leveldb::DB* index, const K& k){
    leveldb::Slice key(k.data(), k.size());
    std::string data;
    leveldb::Status status = GetObject(index, key, &data);
    if(status.IsNotFound()){
      return false;
    }
    return status.ok();
  }

  static inline bool
  HasObject(leveldb::DB* index, const Hash& hash, const Type& type){
    return HasObject(index, ObjectKey(type, hash));
  }

  static inline bool
  HasObject(leveldb::DB* index, const User& user){
    return HasObject(index, UserKey(user));
  }

  template<class K>
  static inline leveldb::Status
  RemoveObject(leveldb::DB* index, const K& k){
    leveldb::WriteOptions writeOpts;
    writeOpts.sync = true;
    leveldb::Slice key(k.data(), k.size());
    return index->Delete(writeOpts, key);
  }

  template<class K>
  static inline void
  RemoveObject(leveldb::WriteBatch& batch, const K& k){
    leveldb::WriteOptions writeOpts;
    writeOpts.sync = true;
    leveldb::Slice key(k.data(), k.size());
    return batch.Delete(key);
  }

  static inline leveldb::Status
  RemoveObject(leveldb::DB* index, const Hash& hash, const Type& type){
    return RemoveObject(index, ObjectKey(type, hash));
  }

  static inline leveldb::Status
  RemoveObject(leveldb::DB* index, const User& user){
    return RemoveObject(index, UserKey(user));
  }

  static inline void
  RemoveObject(leveldb::WriteBatch& batch, const Hash& hash, const Type& type){
    return RemoveObject(batch, ObjectKey(type, hash));
  }

  static inline void
  RemoveObject(leveldb::WriteBatch& batch, const User& user){
    return RemoveObject(batch, UserKey(user));
  }
}

#endif //TOKEN_UTILS_KVSTORE_H
