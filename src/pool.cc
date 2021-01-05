#include <mutex>
#include <glog/logging.h>
#include <condition_variable>
#include "pool.h"
#include "utils/kvstore.h"

namespace Token{
  static std::mutex mutex_;
  static std::condition_variable cond_;
  static ObjectPool::State state_ = ObjectPool::kUninitialized;
  static ObjectPool::Status status_ = ObjectPool::kOk;
  static leveldb::DB* index_ = nullptr;

#define LOCK_GUARD std::lock_guard<std::mutex> guard(mutex_)
#define LOCK std::unique_lock<std::mutex> lock(mutex_)
#define UNLOCK lock.unlock()
#define WAIT cond_.wait(lock)
#define WAIT_TIMED(Timeout) cond_.wait_for(lock, std::chrono::milliseconds((Timeout)))
#define SIGNAL_ONE cond_.notify_one()
#define SIGNAL_ALL cond_.notify_all()

  static inline leveldb::DB*
  GetIndex(){
    return index_;
  }

  static inline std::string
  GetDataDirectory(){
    return TOKEN_BLOCKCHAIN_HOME + "/pool";
  }

  static inline std::string
  GetIndexFilename(){
    return GetDataDirectory() + "/index";
  }

  static inline std::string
  GetDataFilename(const Hash& hash){
    std::stringstream filename;
    filename << GetDataDirectory() << "/" << hash << ".dat";
    return filename.str();
  }

  static inline std::string
  GetBlockFilename(const Hash& hash){
    std::stringstream filename;
    filename << GetDataDirectory() << "/b" << hash << ".dat";
    return filename.str();
  }

  static inline std::string
  GetTransactionFilename(const Hash& hash){
    std::stringstream filename;
    filename << GetDataDirectory() << "/t" << hash << ".dat";
    return filename.str();
  }

  static inline std::string
  GetUnclaimedTransactionFilename(const Hash& hash){
    std::stringstream filename;
    filename << GetDataDirectory() << "/u" << hash << ".dat";
    return filename.str();
  }

  ObjectPool::State ObjectPool::GetState(){
    LOCK_GUARD;
    return state_;
  }

  void ObjectPool::SetState(const State& state){
    LOCK;
    state_ = state;
    UNLOCK;
    SIGNAL_ALL;
  }

  ObjectPool::Status ObjectPool::GetStatus(){
    LOCK_GUARD;
    return status_;
  }

  void ObjectPool::SetStatus(const Status& status){
    LOCK;
    status_ = status;
    UNLOCK;
    SIGNAL_ALL;
  }



  bool ObjectPool::Initialize(){
    if(IsInitialized()){
      LOG(WARNING) << "cannot re-initialize the object pool.";
      return false;
    }

    LOG(INFO) << "initializing the object pool....";
    SetState(ObjectPool::kInitializing);
    if(!FileExists(GetDataDirectory())){
      if(!CreateDirectory(GetDataDirectory())){
        LOG(WARNING) << "cannot create the object pool directory: " << GetDataDirectory();
        return false;
      }
    }

    leveldb::Options options;
    options.comparator = new ObjectHashKey::Comparator();
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

    LOCK;
    WAIT_TIMED(timeout_ms);
    UNLOCK;

    if(!HasBlock(hash)){
      LOG(WARNING) << hash << " wasn't resolved before timeout.";
      return false;
    }
    return true;
  }

  bool ObjectPool::PutBlock(const Hash& hash, const BlockPtr& val){
    if(HasBlock(hash)){
      LOG(WARNING) << "cannot overwrite existing object pool data for: " << hash;
      SetStatus(ObjectPool::kWarning);
      return false;
    }

    leveldb::Status status;
    if(!(status = PutObject(GetIndex(), hash, val)).ok()){
      LOG(WARNING) << "cannot index object " << hash << ": " << status.ToString();
      SetStatus(ObjectPool::kWarning);
      return false;
    }

    SIGNAL_ALL;
    LOG(INFO) << "indexed object: " << hash;
    return true;
  }

  bool ObjectPool::PutTransaction(const Hash& hash, const TransactionPtr& val){
    if(HasTransaction(hash)){
      LOG(WARNING) << "cannot overwrite existing object pool data for: " << hash;
      SetStatus(ObjectPool::kWarning);
      return false;
    }

    leveldb::Status status;
    if(!(status = PutObject(GetIndex(), hash, val)).ok()){
      LOG(WARNING) << "cannot index object " << hash << ": " << status.ToString();
      SetStatus(ObjectPool::kWarning);
      return false;
    }

    SIGNAL_ALL;
    LOG(INFO) << "indexed object: " << hash;
    return true;
  }

  bool ObjectPool::PutUnclaimedTransaction(const Hash& hash, const UnclaimedTransactionPtr& val){
    if(HasUnclaimedTransaction(hash)){
      LOG(WARNING) << "cannot overwrite existing object pool data for: " << hash;
      SetStatus(ObjectPool::kWarning);
      return false;
    }

    leveldb::Status status;
    if(!(status = PutObject(GetIndex(), hash, val)).ok()){
      LOG(WARNING) << "cannot index object " << hash << ": " << status.ToString();
      return false;
    }

    SIGNAL_ALL;
    LOG(INFO) << "indexed object: " << hash;
    return true;
  }

  bool ObjectPool::HasUnclaimedTransactions(const User& user){
    std::string key = user.Get();
    std::string value;
    leveldb::Status status;
    return GetIndex()->Get(leveldb::ReadOptions(), key, &value).ok();
  }

  bool ObjectPool::PutHashList(const User& user, const HashList& hashes){
    std::string key = user.Get();

    int64_t val_size = GetBufferSize(hashes);
    uint8_t val_data[val_size];
    Encode(hashes, val_data, val_size);
    leveldb::Slice value((char*) val_data, val_size);

    leveldb::WriteOptions opts;
    opts.sync = true;
    leveldb::Status status;
    if(!(status = GetIndex()->Put(opts, key, value)).ok()){
      LOG(WARNING) << "cannot index hash list for user " << user << ": " << status.ToString();
      return false;
    }

    LOG(INFO) << "indexed hash list for user: " << user;
    return true;
  }

  bool ObjectPool::GetBlocks(HashList& hashes){
    leveldb::Iterator* it = GetIndex()->NewIterator(leveldb::ReadOptions());
    for(it->SeekToFirst(); it->Valid(); it->Next()){
      ObjectHashKey key(it->key());
      if(key.IsBlock())
        hashes.insert(key.GetHash());
    }
    return true;
  }

  bool ObjectPool::GetBlocks(JsonString& json){
    leveldb::Iterator* it = GetIndex()->NewIterator(leveldb::ReadOptions());

    JsonWriter writer(json);
    writer.StartArray();
    for(it->SeekToFirst(); it->Valid(); it->Next()){
      ObjectHashKey key(it->key());
      if(key.IsBlock()){
        Hash hash = key.GetHash();
        std::string hex = hash.HexString();
        writer.String(hex.data(), 64);
      }
    }
    writer.EndArray();
    return true;
  }

  bool ObjectPool::GetTransactions(HashList& hashes){
    leveldb::Iterator* it = GetIndex()->NewIterator(leveldb::ReadOptions());
    for(it->SeekToFirst(); it->Valid(); it->Next()){
      ObjectHashKey key(it->key());
      if(key.IsTransaction())
        hashes.insert(key.GetHash());
    }
    return true;
  }

  bool ObjectPool::GetTransactions(JsonString& json){
    leveldb::Iterator* it = GetIndex()->NewIterator(leveldb::ReadOptions());

    JsonWriter writer(json);
    writer.StartArray();
    for(it->SeekToFirst(); it->Valid(); it->Next()){
      ObjectHashKey key(it->key());
      if(key.IsTransaction()){
        Hash hash = key.GetHash();
        std::string hex = hash.HexString();
        writer.String(hex.data(), 64);
      }
    }
    writer.EndArray();
    return true;
  }

  bool ObjectPool::GetUnclaimedTransactions(HashList& hashes){
    leveldb::Iterator* it = GetIndex()->NewIterator(leveldb::ReadOptions());
    for(it->SeekToFirst(); it->Valid(); it->Next()){
      ObjectHashKey key(it->key());
      if(key.IsUnclaimedTransaction())
        hashes.insert(key.GetHash());
    }
    return true;
  }

  bool ObjectPool::GetUnclaimedTransactions(JsonString& json){
    leveldb::Iterator* it = GetIndex()->NewIterator(leveldb::ReadOptions());

    JsonWriter writer(json);
    writer.StartArray();
    for(it->SeekToFirst(); it->Valid(); it->Next()){
      ObjectHashKey key(it->key());
      if(key.IsUnclaimedTransaction()){
        Hash hash = key.GetHash();
        std::string hex = hash.HexString();
        writer.String(hex.data(), 64);
      }
    }
    writer.EndArray();
    return true;
  }

  bool ObjectPool::GetUnclaimedTransactionsFor(const User& user, HashList& hashes){
    leveldb::Iterator* it = GetIndex()->NewIterator(leveldb::ReadOptions());
    for(it->SeekToFirst(); it->Valid(); it->Next()){
      ObjectHashKey key(it->key());
      if(!key.IsUnclaimedTransaction())
        continue;

      BufferPtr val = Buffer::From(it->value());
      UnclaimedTransactionPtr value = UnclaimedTransaction::NewInstance(val);
      if(value->GetUser() == user)
        hashes.insert(key.GetHash());
    }
    return true;
  }

  bool ObjectPool::GetUnclaimedTransactionsFor(const User& user, JsonString& json){
    std::string key = user.str();
    std::string value;

    leveldb::ReadOptions opts;
    leveldb::Status status;
    if(!(status = GetIndex()->Get(opts, key, &value)).ok()){
      LOG(WARNING) << "cannot get hash list for " << user << ": " << status.ToString();
      return false;
    }

    int64_t size = (int64_t) value.size();
    uint8_t* bytes = (uint8_t*)value.data();

    JsonWriter writer(json);
    writer.StartArray();
    int64_t offset = 0;
    while(offset < size){
      Hash hash(&bytes[offset], Hash::kSize);
      std::string hex = hash.HexString();
      writer.String(hex.data(), 64);
      offset += Hash::kSize;
    }
    writer.EndArray();
    return true;
  }

  bool ObjectPool::VisitBlocks(ObjectPoolBlockVisitor* vis){
    if(!vis)
      return false;

    return false;
  }

  bool ObjectPool::VisitTransactions(ObjectPoolTransactionVisitor* vis){
    leveldb::Iterator* it = GetIndex()->NewIterator(leveldb::ReadOptions());
    for(it->SeekToFirst(); it->Valid(); it->Next()){
      ObjectHashKey key(it->key());
      if(!key.IsTransaction())
        continue;

      BufferPtr buff = Buffer::From(it->value());
      TransactionPtr val = Transaction::NewInstance(buff);
      if(!vis->Visit(val))
        return false;
    }
    delete it;
    return true;
  }

  bool ObjectPool::VisitUnclaimedTransactions(ObjectPoolUnclaimedTransactionVisitor* vis){
    leveldb::Iterator* it = GetIndex()->NewIterator(leveldb::ReadOptions());
    for(it->SeekToFirst(); it->Valid(); it->Next()){
      ObjectHashKey key(it->key());
      if(!key.IsUnclaimedTransaction())
        continue;

      BufferPtr buff = Buffer::From(it->value());
      UnclaimedTransactionPtr val = UnclaimedTransaction::NewInstance(buff);
      if(!vis->Visit(val))
        return false;
    }
    delete it;
    return true;
  }

  BlockPtr ObjectPool::GetBlock(const Hash& hash){
    std::string data;

    leveldb::Status status;
    if(!(status = GetBlockObject(GetIndex(), hash, &data)).ok()){
      LOG(WARNING) << "cannot get " << hash << ": " << status.ToString();
      return BlockPtr(nullptr);
    }

    BufferPtr buff = Buffer::From(data);
    return Block::NewInstance(buff);
  }

  TransactionPtr ObjectPool::GetTransaction(const Hash& hash){
    std::string data;

    leveldb::ReadOptions opts;
    leveldb::Status status;
    if(!(status = GetTransactionObject(GetIndex(), hash, &data)).ok()){
      LOG(WARNING) << "cannot get " << hash << ": " << status.ToString();
      return TransactionPtr(nullptr);
    }

    BufferPtr buff = Buffer::From(data);
    return Transaction::NewInstance(buff);
  }

  UnclaimedTransactionPtr ObjectPool::GetUnclaimedTransaction(const Hash& hash){
    std::string data;

    leveldb::Status status;
    if(!(status = ::Token::GetUnclaimedTransactionObject(GetIndex(), hash, &data)).ok()){
      LOG(WARNING) << "cannot get " << hash << ": " << status.ToString();
      return UnclaimedTransactionPtr(nullptr);
    }

    BufferPtr buff = Buffer::From(data);
    return UnclaimedTransaction::NewInstance(buff);
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
      ObjectHashKey key(it->key());
      if(key.IsValid())
        count++;
    }

    delete it;
    return count;
  }

  bool ObjectPool::GetStats(JsonString& json){
    int64_t num_blocks = 0;
    int64_t num_transactions = 0;
    int64_t num_unclaimed_transactions = 0;

    leveldb::Iterator* it = GetIndex()->NewIterator(leveldb::ReadOptions());
    for(it->SeekToFirst(); it->Valid(); it->Next()){
      ObjectHashKey key(it->key());
      if(key.IsBlock()){
        num_blocks++;
      } else if(key.IsTransaction()){
        num_transactions++;
      } else if(key.IsUnclaimedTransaction()){
        num_unclaimed_transactions++;
      }
    }
    delete it;

    JsonWriter writer(json);
    writer.StartObject();
    {
      SetField(writer, "NumberOfBlocks", num_blocks);
      SetField(writer, "NumberOfTransactions", num_transactions);
      SetField(writer, "NumberOfUnclaimedTransactions", num_unclaimed_transactions);
    }
    writer.EndObject();
    return true;
  }

  UnclaimedTransactionPtr ObjectPool::FindUnclaimedTransaction(const Input& input){
    leveldb::Iterator* it = GetIndex()->NewIterator(leveldb::ReadOptions());
    for(it->SeekToFirst(); it->Valid(); it->Next()){
      ObjectHashKey key(it->key());
      if(!key.IsUnclaimedTransaction())
        continue;

      BufferPtr val = Buffer::From(it->value());
      UnclaimedTransactionPtr value = UnclaimedTransaction::NewInstance(val);
      if(value->GetTransaction() == input.GetTransactionHash() && value->GetIndex() == input.GetOutputIndex())
        return value;
    }
    delete it;
    return UnclaimedTransactionPtr(nullptr);
  }

  bool ObjectPool::HasUnclaimedTransaction(const Hash& hash){
    return HasObject(GetIndex(), hash, Object::Type::kUnclaimedTransaction);
  }

  bool ObjectPool::HasTransaction(const Hash& hash){
    return HasObject(GetIndex(), hash, Object::Type::kTransaction);
  }

  bool ObjectPool::HasBlock(const Hash& hash){
    return HasObject(GetIndex(), hash, Object::Type::kBlock);
  }

  int64_t ObjectPool::GetNumberOfBlocks(){
    int64_t count = 0;
    leveldb::Iterator* it = GetIndex()->NewIterator(leveldb::ReadOptions());
    for(it->SeekToFirst(); it->Valid(); it->Next()){
      ObjectHashKey key(it->key());
      if(key.IsBlock())
        count++;
    }
    delete it;
    return count;
  }

  int64_t ObjectPool::GetNumberOfTransactions(){
    int64_t count = 0;
    leveldb::Iterator* it = GetIndex()->NewIterator(leveldb::ReadOptions());
    for(it->SeekToFirst(); it->Valid(); it->Next()){
      ObjectHashKey key(it->key());
      if(key.IsTransaction())
        count++;
    }
    delete it;
    return count;
  }

  int64_t ObjectPool::GetNumberOfUnclaimedTransactions(){
    int64_t count = 0;
    leveldb::Iterator* it = GetIndex()->NewIterator(leveldb::ReadOptions());
    for(it->SeekToFirst(); it->Valid(); it->Next()){
      ObjectHashKey key(it->key());
      if(key.IsUnclaimedTransaction())
        count++;
    }
    delete it;
    return count;
  }

  bool ObjectPool::RemoveBlock(const Hash& hash){
    leveldb::Status status;
    if(!(status = RemoveObject(GetIndex(), hash, Object::Type::kBlock)).ok()){
      LOG(WARNING) << "cannot remove block " << hash << " from pool.";
      return false;
    }
    return true;
  }

  bool ObjectPool::RemoveTransaction(const Hash& hash){
    leveldb::Status status;
    if(!(status = RemoveObject(GetIndex(), hash, Object::Type::kTransaction)).ok()){
      LOG(WARNING) << "cannot remove transaction " << hash << " from pool.";
      return false;
    }
    return true;
  }

  bool ObjectPool::RemoveUnclaimedTransaction(const Hash& hash){
    leveldb::Status status;
    if(!(status = RemoveObject(GetIndex(), hash, Object::Type::kUnclaimedTransaction)).ok()){
      LOG(WARNING) << "cannot remove unclaimed transaction " << hash << " from pool.";
      return false;
    }
    return true;
  }
}