#include <mutex>
#include <glog/logging.h>
#include "pool.h"
#include "configuration.h"
#include "atomic/relaxed_atomic.h"

namespace token{

  static inline std::string
  GetBlockChainHome(){
    return TOKEN_BLOCKCHAIN_HOME;
  }


#define POOL_LOG(LevelName) \
  LOG(LevelName) << "[ObjectPool] "

  leveldb::Status ObjectPool::InitializeIndex(const std::string& filename){
    if(IsInitialized())
      return leveldb::Status::NotSupported("Cannot re-initialize the object pool.");

#ifdef TOKEN_DEBUG
    POOL_LOG(INFO) << "initializing in " << filename << "....";
#endif//TOKEN_DEBUG
    SetState(ObjectPool::kInitializing);

    leveldb::Options options;
    options.comparator = new ObjectPool::Comparator();
    options.create_if_missing = true;

    leveldb::Status status;
    if(!(status = leveldb::DB::Open(options, filename, &index_)).ok()){
      POOL_LOG(WARNING) << "couldn't initialize the index: " << status.ToString();
      return status;
    }

    SetState(ObjectPool::kInitialized);
#ifdef TOKEN_DEBUG
    POOL_LOG(INFO) << "initialized.";
#endif//TOKEN_DEBUG
    return leveldb::Status::OK();
  }

  leveldb::Status ObjectPool::Write(const leveldb::WriteBatch& update) const{
    leveldb::WriteOptions opts;
    opts.sync = true;
    return GetIndex()->Write(opts, (leveldb::WriteBatch*)&update);
  }

  int64_t ObjectPool::GetNumberOfObjects() const{
    int64_t count = 0;

    leveldb::Iterator* it = GetIndex()->NewIterator(leveldb::ReadOptions());
    for(it->SeekToFirst(); it->Valid(); it->Next()){
      PoolKey key(it->key());
      if(key.tag().IsValid())
        count++;
    }

    delete it;
    return count;
  }

  UnclaimedTransactionPtr ObjectPool::FindUnclaimedTransaction(const Input& input) const{
    LOG(INFO) << "searching for: " << input;
    leveldb::Iterator* it = GetIndex()->NewIterator(leveldb::ReadOptions());
    for(it->SeekToFirst(); it->Valid(); it->Next()){
      PoolKey key(it->key());
      ObjectTag tag = key.tag();
      if(!tag.IsValid() || !tag.IsUnclaimedTransactionType())
        continue;

      BufferPtr val = Buffer::From(it->value());
      UnclaimedTransactionPtr value = UnclaimedTransaction::FromBytes(val);

      TransactionReference& r1 = value->GetReference();
      const TransactionReference& r2 = input.GetReference();
      if(r1 == r2)
        return value;
    }
    delete it;
    return UnclaimedTransactionPtr(nullptr);
  }

#define DEFINE_PRINT_TYPE(Name) \
  bool ObjectPool::Print##Name##s(const google::LogSeverity& severity) const{ \
    LOG_AT_LEVEL(severity) << "object pool " << #Name << "s:";         \
    leveldb::Iterator* it = GetIndex()->NewIterator(leveldb::ReadOptions()); \
    for(it->SeekToFirst(); it->Valid(); it->Next()){                   \
      PoolKey key(it->key());   \
      ObjectTag tag = key.tag();\
      if(!tag.IsValid() || !tag.Is##Name##Type())                      \
        continue;               \
      Name##Ptr value = Name::FromBytes(Buffer::From(it->value()));    \
      LOG_AT_LEVEL(severity) << " - " << value->ToString();            \
    }                           \
    delete it;                  \
    return true;                \
  }
  FOR_EACH_POOL_TYPE(DEFINE_PRINT_TYPE)
#undef DEFINE_PRINT_TYPE

#define DEFINE_PUT_TYPE(Name) \
  bool ObjectPool::Put##Name(const Hash& hash, const Name##Ptr& val) const{ \
    if(Has##Name(hash)){      \
      LOG(WARNING) << "cannot overwrite existing object pool data for: " << hash; \
      return false;           \
    }                         \
    leveldb::WriteOptions options;                                    \
    options.sync = true;      \
    PoolKey key(val->GetType(), val->GetBufferSize(), hash);          \
    BufferPtr buffer = val->ToBuffer();                               \
    leveldb::Status status;   \
    if(!(status = GetIndex()->Put(options, (const leveldb::Slice&)key, buffer->operator leveldb::Slice())).ok()){ \
      LOG(WARNING) << "cannot index object " << hash << ": " << status.ToString();\
      return false;           \
    }                         \
    LOG(INFO) << "indexed object " << hash;                           \
    return true;              \
  }
  FOR_EACH_POOL_TYPE(DEFINE_PUT_TYPE)
#undef DEFINE_PUT_TYPE

#define DEFINE_GET_TYPE(Name) \
  Name##Ptr ObjectPool::Get##Name(const Hash& hash) const{ \
    PoolKey key(Type::k##Name, 0, hash);             \
    std::string data;         \
    leveldb::Status status;   \
    if(!(status = GetIndex()->Get(leveldb::ReadOptions(), (const leveldb::Slice&)key, &data)).ok()){ \
      LOG(WARNING) << "cannot get " << hash << ": " << status.ToString(); \
      return Name##Ptr(nullptr);                     \
    }                         \
    BufferPtr buff = Buffer::From(data);             \
    return Name::FromBytes(buff);                    \
  }
  FOR_EACH_POOL_TYPE(DEFINE_GET_TYPE)
#undef DEFINE_GET_TYPE

#define DEFINE_HAS_TYPE(Name) \
  bool ObjectPool::Has##Name(const Hash& hash) const{ \
    std::string data;                          \
    PoolKey key(Type::k##Name, 0, hash);        \
    return GetIndex()->Get(leveldb::ReadOptions(), (const leveldb::Slice&)key, &data).ok(); \
  }
  FOR_EACH_POOL_TYPE(DEFINE_HAS_TYPE);
#undef DEFINE_HAS_TYPE

#define DEFINE_GET_TYPE_COUNT(Name) \
  int64_t ObjectPool::GetNumberOf##Name##s() const{ \
    int64_t count = 0;              \
    leveldb::Iterator* iter = GetIndex()->NewIterator(leveldb::ReadOptions()); \
    for(iter->SeekToFirst(); iter->Valid(); iter->Next()){                     \
      PoolKey key(iter->key());     \
      ObjectTag tag = key.tag();    \
      if(tag.IsValid() && tag.Is##Name##Type())            \
        count++;                    \
    }                               \
    delete iter;                    \
    return count;                   \
  }
  FOR_EACH_POOL_TYPE(DEFINE_GET_TYPE_COUNT)
#undef DEFINE_GET_TYPE_COUNT

#define DEFINE_REMOVE_TYPE(Name) \
  bool ObjectPool::Remove##Name(const Hash& hash) const{ \
    PoolKey key(Type::k##Name, 0, hash); \
    leveldb::WriteOptions options;                 \
    options.sync = true;         \
    leveldb::Status status;      \
    if(!(status = GetIndex()->Delete(options, (const leveldb::Slice&)key)).ok()){ \
      LOG(WARNING) << "cannot remove " << hash << " from pool: " << status.ToString(); \
      return false;              \
    }                            \
    return true;                 \
  }
  FOR_EACH_POOL_TYPE(DEFINE_REMOVE_TYPE)
#undef DEFINE_REMOVE_TYPE

#define DEFINE_VISIT_TYPE(Name) \
  bool ObjectPool::Visit##Name##s(ObjectPool##Name##Visitor* vis) const{ \
    leveldb::Iterator* iter = GetIndex()->NewIterator(leveldb::ReadOptions()); \
    for(iter->SeekToFirst(); iter->Valid(); iter->Next()){         \
      PoolKey key(iter->key()); \
      ObjectTag tag = key.tag();\
      if(!tag.IsValid() || !tag.Is##Name##Type())       \
        continue;               \
      BufferPtr buffer = Buffer::From(iter->value());              \
      Name##Ptr val = Name::FromBytes(buffer);                     \
      if(!vis->Visit(val))     \
        return false;           \
    }                           \
    delete iter;                \
    return true;                \
  }
  FOR_EACH_POOL_TYPE(DEFINE_VISIT_TYPE)
#undef DEFINE_VISIT_TYPE

#define DEFINE_GET_TYPE_HASHES(Name) \
  bool ObjectPool::Get##Name##s(Json::Writer& writer) const{ \
    leveldb::Iterator* iter = GetIndex()->NewIterator(leveldb::ReadOptions()); \
    writer.StartArray();             \
    for(iter->SeekToFirst(); iter->Valid(); iter->Next()){                     \
      PoolKey key(iter->key());    \
      ObjectTag tag = key.tag();     \
      if(tag.IsValid() && tag.Is##Name##Type()){            \
        Hash hash = key.GetHash();   \
        std::string hex = hash.HexString();          \
        writer.String(hex.data(), 64);               \
      }                              \
    }                                \
    writer.EndArray();               \
    delete iter;                     \
    return true;                     \
  }
  FOR_EACH_POOL_TYPE(DEFINE_GET_TYPE_HASHES)
#undef DEFINE_GET_TYPE_HASHES

#define DEFINE_GET_TYPE_HASHES(Name) \
  bool ObjectPool::Get##Name##s(HashList& hashes) const{ \
    leveldb::Iterator* iter = GetIndex()->NewIterator(leveldb::ReadOptions()); \
    for(iter->SeekToFirst(); iter->Valid(); iter->Next()){                     \
      PoolKey key(iter->key());      \
      ObjectTag tag = key.tag();     \
      if(tag.IsValid() && tag.Is##Name##Type()){   \
        hashes.push_back(key.GetHash());           \
      }                              \
    }                                \
    delete iter;                     \
    return true;                     \
  }
  FOR_EACH_POOL_TYPE(DEFINE_GET_TYPE_HASHES)
#undef DEFINE_GET_TYPE_HASHES

#define DEFINE_HAS_TYPE(Name) \
  bool ObjectPool::Has##Name##s() const{ \
    leveldb::Iterator* iter = GetIndex()->NewIterator(leveldb::ReadOptions()); \
    for(iter->SeekToFirst(); iter->Valid(); iter->Next()){                     \
      PoolKey key(iter->key());    \
      ObjectTag tag = key.tag();   \
      if(tag.IsValid() && tag.Is##Name##Type()){                               \
        delete iter;          \
        return true;          \
      }                       \
    }                         \
    delete iter;              \
    return false;             \
  }
  FOR_EACH_POOL_TYPE(DEFINE_HAS_TYPE)
#undef DEFINE_HAS_TYPE

  ObjectPoolPtr ObjectPool::GetInstance(){
    static ObjectPoolPtr instance = std::make_shared<ObjectPool>();
    return instance;
  }

  bool ObjectPool::Initialize(const std::string& filename){
    leveldb::Status status;
    if(!(status = GetInstance()->InitializeIndex(filename)).ok()){
      LOG(ERROR) << "couldn't initialize the object pool in " << filename << ": " << status.ToString();
      return false;
    }
    return true;
  }
}