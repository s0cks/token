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

namespace Token{
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

  class ObjectHashKey : public KeyType{
   public:
    static const int16_t kMagic = 0xFAFE;

    enum Layout{
      kMagicOffset = 0,
      kBytesForMagic = sizeof(int16_t),

      kTypeOffset = kMagicOffset + kBytesForMagic,
      kBytesForType = sizeof(Object::Type),

      kHashOffset = kTypeOffset + kBytesForType,
      kBytesForHash = Hash::kSize,

      kTotalSize = kHashOffset + kBytesForHash,
    };

    class Comparator : public leveldb::Comparator{
     public:
      int Compare(const leveldb::Slice& a, const leveldb::Slice& b) const{
        ObjectHashKey k1(a);
        ObjectHashKey k2(b);

        Object::Type t1 = k1.GetType();
        Object::Type t2 = k2.GetType();
        if(t1 < t2){
          return -1;
        } else if(t1 > t2){
          return +1;
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
    ObjectHashKey(const Object::Type& type, const Hash& hash):
      data_(){
      memcpy(&data_[kMagicOffset], &kMagic, kBytesForMagic);
      memcpy(&data_[kTypeOffset], &type, kBytesForType);
      memcpy(&data_[kHashOffset], hash.data(), kBytesForHash);
    }
    ObjectHashKey(const leveldb::Slice& value):
      data_(){
      if(value.size() < kTotalSize){
        LOG(WARNING) << "copying partial key of size " << value.size() << "(value.size() - kTotalSize).";
      } else if(value.size() > kTotalSize){
        LOG(WARNING) << "copying partial key of size " << value.size() << "(" << (value.size() - kTotalSize)
                     << " bytes remaining).";
      }
      memcpy(data_, value.data(), std::min(value.size(), (size_t) kTotalSize));
    }
    ObjectHashKey(const std::string& value):
      data_(){
      if(value.size() < kTotalSize){
        LOG(WARNING) << "copying partial key of size " << value.size() << "(value.size() - kTotalSize).";
      } else if(value.size() > kTotalSize){
        LOG(WARNING) << "copying partial key of size " << value.size() << "(" << (value.size() - kTotalSize)
                     << " bytes remaining).";
      }
      memcpy(data_, value.data(), std::min(value.size(), (size_t) kTotalSize));
    }
    ~ObjectHashKey() = default;

    char* data() const{
      return (char*) data_;
    }

    size_t size() const{
      return kTotalSize;
    }

    Object::Type GetType() const{
      return *((Object::Type*) &data_[kTypeOffset]);
    }

    Hash GetHash() const{
      return Hash(&data_[kHashOffset], kBytesForHash);
    }

    int16_t GetMagic() const{
      return *((int16_t*) &data_[kMagicOffset]);
    }

    bool IsValid() const{
      return GetMagic() == kMagic;
    }

    bool IsBlock() const{
      return GetType() == Object::Type::kBlock;
    }

    bool IsTransaction() const{
      return GetType() == Object::Type::kTransaction;
    }

    bool IsUnclaimedTransaction() const{
      return GetType() == Object::Type::kUnclaimedTransaction;
    }

    void operator=(const ObjectHashKey& key){
      memcpy(data_, key.data_, kTotalSize);
    }

    friend bool operator==(const ObjectHashKey& a, const ObjectHashKey& b){
      return memcmp(a.data_, b.data_, kTotalSize) == 0;
    }

    friend bool operator!=(const ObjectHashKey& a, const ObjectHashKey& b){
      return memcmp(a.data_, b.data_, kTotalSize) != 0;
    }

    friend std::ostream& operator<<(std::ostream& stream, const ObjectHashKey& key){
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

  static inline leveldb::Status
  GetBlockObject(leveldb::DB* index, const Hash& hash, std::string* val){
    return GetObject(index, ObjectHashKey(Object::Type::kBlock, hash), val);
  }

  static inline leveldb::Status
  GetTransactionObject(leveldb::DB* index, const Hash& hash, std::string* val){
    return GetObject(index, ObjectHashKey(Object::Type::kTransaction, hash), val);
  }

  static inline leveldb::Status
  GetUnclaimedTransactionObject(leveldb::DB* index, const Hash& hash, std::string* val){
    return GetObject(index, ObjectHashKey(Object::Type::kUnclaimedTransaction, hash), val);
  }

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

  static inline leveldb::Status
  PutObject(leveldb::DB* index, const Hash& hash, const BlockPtr& val){
    return PutObject(index, ObjectHashKey(Object::Type::kBlock, hash), val);
  }

  static inline void
  PutObject(leveldb::WriteBatch& batch, const Hash& hash, const BlockPtr& val){
    return PutObject(batch, ObjectHashKey(Object::Type::kBlock, hash), val);
  }

  static inline leveldb::Status
  PutObject(leveldb::DB* index, const Hash& hash, const TransactionPtr& val){
    return PutObject(index, ObjectHashKey(Object::Type::kTransaction, hash), val);
  }

  static inline void
  PutObject(leveldb::WriteBatch& batch, const Hash& hash, const TransactionPtr& val){
    return PutObject(batch, ObjectHashKey(Object::Type::kTransaction, hash), val);
  }

  static inline leveldb::Status
  PutObject(leveldb::DB* index, const Hash& hash, const UnclaimedTransactionPtr& val){
    return PutObject(index, ObjectHashKey(Object::Type::kUnclaimedTransaction, hash), val);
  }

  static inline void
  PutObject(leveldb::WriteBatch& batch, const Hash& hash, const UnclaimedTransactionPtr& val){
    return PutObject(batch, ObjectHashKey(Object::Type::kUnclaimedTransaction, hash), val);
  }

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
  HasObject(leveldb::DB* index, const Hash& hash, const Object::Type& type){
    return HasObject(index, ObjectHashKey(type, hash));
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
  RemoveObject(leveldb::DB* index, const Hash& hash, const Object::Type& type){
    return RemoveObject(index, ObjectHashKey(type, hash));
  }

  static inline leveldb::Status
  RemoveObject(leveldb::DB* index, const User& user){
    return RemoveObject(index, UserKey(user));
  }

  static inline void
  RemoveObject(leveldb::WriteBatch& batch, const Hash& hash, const Object::Type& type){
    return RemoveObject(batch, ObjectHashKey(type, hash));
  }

  static inline void
  RemoveObject(leveldb::WriteBatch& batch, const User& user){
    return RemoveObject(batch, UserKey(user));
  }
}

#endif //TOKEN_UTILS_KVSTORE_H
