#include <glog/logging.h>
#include "pool.h"
#include "utils/kvstore.h"
#include "utils/relaxed_atomic.h"

namespace Token{
  static RelaxedAtomic<ObjectPool::State> state_ = { ObjectPool::kUninitialized };
  static leveldb::DB* index_ = nullptr;

  static inline leveldb::DB*
  GetIndex(){
    return index_;
  }

  static inline std::string
  GetIndexFilename(){
    return TOKEN_BLOCKCHAIN_HOME + "/pool";
  }

  ObjectPool::State ObjectPool::GetState(){
    return state_;
  }

  void ObjectPool::SetState(const State& state){
    state_ = state;
  }

  bool ObjectPool::Initialize(){
    if(IsInitialized()){
      LOG(WARNING) << "cannot re-initialize the object pool.";
      return false;
    }

    LOG(INFO) << "initializing the object pool....";
    SetState(ObjectPool::kInitializing);
    leveldb::Options options;
    options.comparator = new ObjectKey::Comparator();
    options.create_if_missing = true;

    leveldb::Status status;
    if(!(status = leveldb::DB::Open(options, GetIndexFilename(), &index_)).ok()){
      LOG(WARNING) << "couldn't initialize the object pool index: " << status.ToString();
      return false;
    }

    LOG(INFO) << "object pool initialized.";
    SetState(ObjectPool::kInitialized);
    return true;
  }

  bool ObjectPool::WaitForBlock(const Hash& hash, const int64_t timeout_ms){
    LOG(INFO) << "waiting " << timeout_ms << "ms for: " << hash;
    //TODO: add wait logic
    if(!HasBlock(hash)){
      LOG(WARNING) << hash << " wasn't resolved before timeout.";
      return false;
    }
    return true;
  }

  leveldb::Status ObjectPool::Write(leveldb::WriteBatch* update){
    leveldb::WriteOptions opts;
    opts.sync = true;
    return GetIndex()->Write(opts, update);
  }

  int64_t ObjectPool::GetNumberOfObjects(){
    int64_t count = 0;

    leveldb::Iterator* it = GetIndex()->NewIterator(leveldb::ReadOptions());
    for(it->SeekToFirst(); it->Valid(); it->Next()){
      ObjectKey key(it->key());
      if(key.IsValid()){
        count++;
      }
    }

    delete it;
    return count;
  }

#ifdef TOKEN_DEBUG
  bool ObjectPool::GetStats(Json::Writer& writer){
    int64_t num_blocks = 0;
    int64_t num_transactions = 0;
    int64_t num_unclaimed_transactions = 0;

    leveldb::Iterator* it = GetIndex()->NewIterator(leveldb::ReadOptions());
    for(it->SeekToFirst(); it->Valid(); it->Next()){
      ObjectKey key(it->key());
      if(key.IsBlock()){
        num_blocks++;
      } else if(key.IsTransaction()){
        num_transactions++;
      } else if(key.IsUnclaimedTransaction()){
        num_unclaimed_transactions++;
      }
    }
    delete it;

    writer.StartObject();
    {
      Json::SetField(writer, "NumberOfBlocks", num_blocks);
      Json::SetField(writer, "NumberOfTransactions", num_transactions);
      Json::SetField(writer, "NumberOfUnclaimedTransactions", num_unclaimed_transactions);
    }
    writer.EndObject();
    return true;
  }
#endif//TOKEN_DEBUG

  UnclaimedTransactionPtr ObjectPool::FindUnclaimedTransaction(const Input& input){
    LOG(INFO) << "searching for: " << input;
    leveldb::Iterator* it = GetIndex()->NewIterator(leveldb::ReadOptions());
    for(it->SeekToFirst(); it->Valid(); it->Next()){
      ObjectKey key(it->key());
      if(!key.IsUnclaimedTransaction()){
        continue;
      }

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

#define DEFINE_PUT_TYPE(Name) \
  bool ObjectPool::Put##Name(const Hash& hash, const Name##Ptr& val){ \
    if(Has##Name(hash)){      \
      LOG(WARNING) << "cannot overwrite existing object pool data for: " << hash; \
      return false;           \
    }                         \
    leveldb::Status status;   \
    if(!(status = PutObject(GetIndex(), hash, val)).ok()){            \
      LOG(WARNING) << "cannot index object " << hash << ": " << status.ToString();\
      return false;           \
    }                         \
    LOG(INFO) << "indexed object " << hash;                           \
    return true;              \
  }
  FOR_EACH_POOL_TYPE(DEFINE_PUT_TYPE)
#undef DEFINE_PUT_TYPE

#define DEFINE_GET_TYPE(Name) \
  Name##Ptr ObjectPool::Get##Name(const Hash& hash){ \
    std::string data;         \
    leveldb::Status status;   \
    if(!(status = Get##Name##Object(GetIndex(), hash, &data)).ok()){ \
      LOG(WARNING) << "cannot get " << hash << ": " << status.ToString(); \
      return Name##Ptr(nullptr);                     \
    }                         \
    BufferPtr buff = Buffer::From(data);             \
    return Name::FromBytes(buff);                    \
  }
  FOR_EACH_POOL_TYPE(DEFINE_GET_TYPE)
#undef DEFINE_GET_TYPE

#define DEFINE_HAS_TYPE(Name) \
  bool ObjectPool::Has##Name(const Hash& hash){ \
    return HasObject(GetIndex(), hash, Object::Type::k##Name##Type); \
  }
  FOR_EACH_POOL_TYPE(DEFINE_HAS_TYPE);
#undef DEFINE_HAS_TYPE

#define DEFINE_GET_TYPE_COUNT(Name) \
  int64_t ObjectPool::GetNumberOf##Name##s(){ \
    int64_t count = 0;              \
    leveldb::Iterator* iter = GetIndex()->NewIterator(leveldb::ReadOptions()); \
    for(iter->SeekToFirst(); iter->Valid(); iter->Next()){                     \
      ObjectKey key(iter->key());     \
      if(key.Is##Name())            \
        count++;                    \
    }                               \
    delete iter;                    \
    return count;                   \
  }
  FOR_EACH_POOL_TYPE(DEFINE_GET_TYPE_COUNT)
#undef DEFINE_GET_TYPE_COUNT

#define DEFINE_REMOVE_TYPE(Name) \
  bool ObjectPool::Remove##Name(const Hash& hash){ \
    leveldb::Status status;      \
    if(!(status = RemoveObject(GetIndex(), hash, Object::Type::k##Name##Type)).ok()){ \
      LOG(WARNING) << "cannot remove " << hash << " from pool: " << status.ToString(); \
      return false;              \
    }                            \
    return true;                 \
  }
  FOR_EACH_POOL_TYPE(DEFINE_REMOVE_TYPE)
#undef DEFINE_REMOVE_TYPE

#define DEFINE_VISIT_TYPE(Name) \
  bool ObjectPool::Visit##Name##s(ObjectPool##Name##Visitor* vis){ \
    leveldb::Iterator* iter = GetIndex()->NewIterator(leveldb::ReadOptions()); \
    for(iter->SeekToFirst(); iter->Valid(); iter->Next()){         \
      ObjectKey key(iter->key());                                  \
      if(!key.Is##Name())       \
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
  bool ObjectPool::Get##Name##s(Json::Writer& writer){ \
    leveldb::Iterator* iter = GetIndex()->NewIterator(leveldb::ReadOptions()); \
    writer.StartArray();             \
    for(iter->SeekToFirst(); iter->Valid(); iter->Next()){                     \
      ObjectKey key(iter->key());    \
      if(key.Is##Name()){            \
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
}