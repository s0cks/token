#include <glog/logging.h>

#include "pool.h"
#include "buffer.h"
#include "configuration.h"
#include "atomic/relaxed_atomic.h"

namespace token{

  static inline std::string
  GetBlockChainHome(){
    return TOKEN_BLOCKCHAIN_HOME;
  }

  ObjectPool::ObjectPool(const std::string& filename):
      state_(State::kUninitialized),
      index_(nullptr){
    leveldb::Status status;
    if(!(status = InitializeIndex(filename)).ok()){
      LOG(FATAL) << "cannot initialize object pool index: " << status.ToString();
      return;
    }
  }

  leveldb::Status ObjectPool::InitializeIndex(const std::string& filename){
    if(IsInitialized())
      return leveldb::Status::NotSupported("Cannot re-initialize the object pool.");

    DLOG(INFO) << "initializing in: " << filename;
    SetState(ObjectPool::kInitializing);

    leveldb::Options options;
    options.comparator = new ObjectPool::Comparator();
    options.create_if_missing = true;

    leveldb::Status status;
    if(!(status = leveldb::DB::Open(options, filename, &index_)).ok())
      return status;

    SetState(ObjectPool::kInitialized);
    DLOG(INFO) << "initialized.";
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
      ObjectKey key(it->key());
      count++;
    }

    delete it;
    return count;
  }

  UnclaimedTransactionPtr ObjectPool::FindUnclaimedTransaction(const Input& input) const{
    leveldb::Iterator* it = GetIndex()->NewIterator(leveldb::ReadOptions());
    for(it->SeekToFirst(); it->Valid(); it->Next()){
      ObjectKey key(it->key());
      if(key.type() != Type::kUnclaimedTransaction)
        continue;

      BufferPtr val = internal::CopyBufferFrom(it->value());
      UnclaimedTransactionPtr value = UnclaimedTransaction::Decode(val);

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
    for(it->SeekToFirst(); it->Valid(); it->Next()){                          \
      ObjectKey key(it->key()); \
      if(key.type() != Type::k##Name)                                      \
        continue;               \
      BufferPtr val = internal::CopyBufferFrom(it->value()); \
      Name##Ptr value = Name::Decode(val);                                 \
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
      DLOG(WARNING) << "cannot overwrite existing object pool data for: " << hash; \
      return false;           \
    }                         \
    leveldb::WriteOptions options;                                          \
    options.sync = true;      \
    ObjectKey key(val->type(), hash);                                       \
    BufferPtr buffer = val->ToBuffer();                                     \
    leveldb::Status status;   \
    if(!(status = GetIndex()->Put(options, (const leveldb::Slice&)key, buffer->operator leveldb::Slice())).ok()){ \
      LOG(ERROR) << "cannot index " << hash << ": " << status.ToString();          \
      return false;           \
    }                         \
    DLOG(INFO) << "indexed object " << hash;                           \
    return true;              \
  }
  FOR_EACH_POOL_TYPE(DEFINE_PUT_TYPE)
#undef DEFINE_PUT_TYPE

#define DEFINE_GET_TYPE(Name) \
  Name##Ptr ObjectPool::Get##Name(const Hash& hash) const{ \
    ObjectKey key(Type::k##Name, hash); \
    std::string data;         \
    leveldb::Status status;   \
    if(!(status = GetIndex()->Get(leveldb::ReadOptions(), (const leveldb::Slice&)key, &data)).ok()){ \
      LOG(WARNING) << "cannot get " << hash << ": " << status.ToString(); \
      return Name##Ptr(nullptr);                           \
    }                         \
    BufferPtr buff = internal::CopyBufferFrom(data);                   \
    return Name::Decode(buff);                          \
  }
  FOR_EACH_POOL_TYPE(DEFINE_GET_TYPE)
#undef DEFINE_GET_TYPE

#define DEFINE_HAS_TYPE(Name) \
  bool ObjectPool::Has##Name(const Hash& hash) const{ \
    std::string data;         \
    ObjectKey key(Type::k##Name, hash);                \
    return GetIndex()->Get(leveldb::ReadOptions(), (const leveldb::Slice&)key, &data).ok(); \
  }
  FOR_EACH_POOL_TYPE(DEFINE_HAS_TYPE);
#undef DEFINE_HAS_TYPE

#define DEFINE_GET_TYPE_COUNT(Name) \
  int64_t ObjectPool::GetNumberOf##Name##s() const{ \
    int64_t count = 0;              \
    leveldb::Iterator* iter = GetIndex()->NewIterator(leveldb::ReadOptions()); \
    for(iter->SeekToFirst(); iter->Valid(); iter->Next()){                     \
      ObjectKey key(iter->key()); \
      if(key.type() != Type::k##Name)               \
        continue;                   \
      count++;                      \
    }                               \
    delete iter;                    \
    return count;                   \
  }
  FOR_EACH_POOL_TYPE(DEFINE_GET_TYPE_COUNT)
#undef DEFINE_GET_TYPE_COUNT

#define DEFINE_REMOVE_TYPE(Name) \
  bool ObjectPool::Remove##Name(const Hash& hash) const{ \
    ObjectKey key(Type::k##Name, hash);                  \
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

  bool ObjectPool::Accept(PoolVisitor* vis) const{
    leveldb::Iterator* iter = GetIndex()->NewIterator(leveldb::ReadOptions());
    for(iter->SeekToFirst(); iter->Valid(); iter->Next()){
      ObjectKey key(iter->key());
      BufferPtr data = internal::CopyBufferFrom(iter->value());
      codec::DecoderHints hints = codec::kDefaultDecoderHints;
      switch(key.type()){
        case Type::kUnclaimedTransaction:{
          auto val = UnclaimedTransaction::Decode(data, hints);
          if(!vis->Visit(val)){
            delete iter;
            return false;
          }
          continue;
        }
        case Type::kUnsignedTransaction:{
          auto val = UnsignedTransaction::Decode(data, hints);
          if(!vis->Visit(val)){
            delete iter;
            return false;
          }
          continue;
        }
        case Type::kBlock:{
          auto val = Block::Decode(data, hints);
          if(!vis->Visit(val)){
            delete iter;
            return false;
          }
          continue;
        }
        default:{
          delete iter;
          return false;
        }
      }
    }

    delete iter;
    return true;
  }

#define DEFINE_GET_TYPE_HASHES(Name) \
  bool ObjectPool::Get##Name##s(json::Writer& writer) const{ \
    leveldb::Iterator* iter = GetIndex()->NewIterator(leveldb::ReadOptions()); \
    writer.StartArray();             \
    for(iter->SeekToFirst(); iter->Valid(); iter->Next()){                     \
      ObjectKey key(iter->key());    \
      if(key.type() == Type::k##Name){\
        Hash hash = key.hash();   \
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
      ObjectKey key(iter->key());    \
      if(key.type() == Type::k##Name)\
        hashes.push_back(key.hash());                 \
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
      ObjectKey key(iter->key());        \
      if(key.type() == Type::k##Name) {  \
        delete iter;          \
        return true;          \
      }                       \
    }                         \
    delete iter;              \
    return false;             \
  }
  FOR_EACH_POOL_TYPE(DEFINE_HAS_TYPE)
#undef DEFINE_HAS_TYPE

  void ObjectPool::GetPoolStats(PoolStats& stats) const{
    stats.num_blocks = GetNumberOfBlocks();
    stats.num_transactions_unsigned = GetNumberOfUnsignedTransactions();
    stats.num_transactions_unclaimed = GetNumberOfUnclaimedTransactions();
  }
}