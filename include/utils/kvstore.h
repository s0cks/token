#ifndef TOKEN_UTILS_KVSTORE_H
#define TOKEN_UTILS_KVSTORE_H

#include <leveldb/db.h>
#include <leveldb/slice.h>
#include <leveldb/comparator.h>
#include <leveldb/write_batch.h>

#include "key.h"
#include "hash.h"
#include "object.h"
#include "wallet.h"
#include "unclaimed_transaction.h"

namespace token{
  template<class K>
  static inline leveldb::Status
  GetObject(leveldb::DB* index, const K& k, std::string* val){
    leveldb::Slice key(k.data(), k.size());
    leveldb::ReadOptions readOpts;
    return index->Get(readOpts, key, val);
  }

  //TODO: fix PoolKey size
#define DEFINE_GETTER(Name) \
  static inline leveldb::Status \
  Get##Name##Object(leveldb::DB* index, const Hash& hash, std::string* val){ \
    return GetObject(index, PoolKey(Type::k##Name, 0, hash), val); \
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
    return PutObject(index, PoolKey(Type::k##Name, val->GetBufferSize(), hash), val); \
  }                                \
  static inline void   \
  PutObject(leveldb::WriteBatch& batch, const Hash& hash, const Name##Ptr& val){ \
    return PutObject(batch, PoolKey(Type::k##Name, val->GetBufferSize(), hash), val);  \
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
    return HasObject(index, PoolKey(type, 0, hash));
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
    return RemoveObject(index, PoolKey(type, 0, hash));
  }

  static inline leveldb::Status
  RemoveObject(leveldb::DB* index, const User& user){
    return RemoveObject(index, UserKey(user));
  }

  static inline void
  RemoveObject(leveldb::WriteBatch& batch, const Hash& hash, const Type& type){
    return RemoveObject(batch, PoolKey(type, 0, hash));
  }

  static inline void
  RemoveObject(leveldb::WriteBatch& batch, const User& user){
    return RemoveObject(batch, UserKey(user));
  }
}

#endif //TOKEN_UTILS_KVSTORE_H
