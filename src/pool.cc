#include <mutex>
#include <glog/logging.h>
#include <condition_variable>
#include "pool.h"

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

  class BlockChainComparator : public leveldb::Comparator{
   public:
    int Compare(const leveldb::Slice& a, const leveldb::Slice& b) const{
      Hash h1((uint8_t*) a.data());
      Hash h2((uint8_t*) b.data());
      if(h1 < h2)
        return -1;
      else if(h1 > h2)
        return +1;
      return 0;
    }

    const char* Name() const{
      return "ObjectPoolComparator";
    }

    void FindShortestSeparator(std::string* str, const leveldb::Slice& slice) const{}
    void FindShortSuccessor(std::string* str) const{}
  };

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
    options.comparator = new BlockChainComparator();
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

  bool ObjectPool::HasObject(const Hash& hash){
    leveldb::ReadOptions readOpts;
    std::string key = hash.HexString();
    std::string filename;
    return GetIndex()->Get(readOpts, key, &filename).ok();
  }

  bool ObjectPool::HasObjectByType(const Hash& hash, const ObjectTag::Type& type){
    leveldb::ReadOptions readOpts;
    std::string key = hash.HexString();
    std::string filename;

    leveldb::Status status;
    if((status = GetIndex()->Get(readOpts, key, &filename)).IsNotFound())
      return false;
    return false; //TODO: fixme
  }

  bool ObjectPool::WaitForObject(const Hash& hash){
    LOCK;
    while(!HasObject(hash)) WAIT;
    UNLOCK;
    return true;
  }

  bool ObjectPool::RemoveObject(const Hash& hash){
    if(!HasObject(hash)){
      LOG(WARNING) << "cannot remove non-existent object " << hash << " from pool.";
      return false;
    }

    leveldb::WriteOptions writeOpts;
    writeOpts.sync = true;

    std::string key = hash.HexString();
    std::string filename = GetDataFilename(hash);

    leveldb::Status status;
    if(!(status = GetIndex()->Delete(writeOpts, key)).ok()){
      LOG(WARNING) << "couldn't remove object " << hash << " from pool: " << status.ToString();
      SetStatus(ObjectPool::kWarning);
      return false;
    }

    if(!DeleteFile(filename)){
      LOG(WARNING) << "couldn't remove object pool file: " << filename;
      SetStatus(ObjectPool::kWarning);
      return false;
    }

    LOG(INFO) << "remove object " << hash << " from pool.";
    return true;
  }

  leveldb::Status ObjectPool::IndexObject(const Hash& hash, const std::string& filename){
    leveldb::WriteOptions writeOpts;
    writeOpts.sync = true;
    return GetIndex()->Put(writeOpts, hash.HexString(), filename);
  }

  bool ObjectPool::PutObject(const Hash& hash, const BlockPtr& val){
    if(HasObject(hash)){
      LOG(WARNING) << "cannot overwrite existing object pool data for: " << hash;
      SetStatus(ObjectPool::kWarning);
      return false;
    }

    ObjectPoolKey pkey(ObjectTag::kBlock, hash);
    BufferPtr kdata = Buffer::NewInstance(ObjectPoolKey::kSize);
    pkey.Write(kdata);

    int64_t vsize = val->GetBufferSize();
    BufferPtr vdata = Buffer::NewInstance(vsize);
    val->Write(vdata);

    leveldb::Slice key(kdata->data(), ObjectPoolKey::kSize);
    leveldb::Slice value(vdata->data(), vsize);

    leveldb::WriteOptions opts;
    opts.sync = true;
    leveldb::Status status;
    if(!(status = GetIndex()->Put(opts, key, value)).ok()){
      LOG(WARNING) << "cannot index object " << hash << ": " << status.ToString();
      SetStatus(ObjectPool::kWarning);
      return false;
    }

    //TODO: should we signal
    LOG(INFO) << "indexed object: " << hash;
    return true;
  }

  bool ObjectPool::PutObject(const Hash& hash, const TransactionPtr& val){
    if(HasObject(hash)){
      LOG(WARNING) << "cannot overwrite existing object pool data for: " << hash;
      SetStatus(ObjectPool::kWarning);
      return false;
    }

    ObjectPoolKey pkey(ObjectTag::kBlock, hash);
    BufferPtr kdata = Buffer::NewInstance(ObjectPoolKey::kSize);
    pkey.Write(kdata);

    int64_t vsize = val->GetBufferSize();
    BufferPtr vdata = Buffer::NewInstance(vsize);
    val->Write(vdata);

    leveldb::Slice key(kdata->data(), ObjectPoolKey::kSize);
    leveldb::Slice value(vdata->data(), vsize);

    leveldb::WriteOptions opts;
    opts.sync = true;
    leveldb::Status status;
    if(!(status = GetIndex()->Put(opts, key, value)).ok()){
      LOG(WARNING) << "cannot index object " << hash << ": " << status.ToString();
      SetStatus(ObjectPool::kWarning);
      return false;
    }

    //TODO: should we signal
    LOG(INFO) << "indexed object: " << hash;
    return true;
  }

  bool ObjectPool::HasHashList(const User& user){
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

  bool ObjectPool::GetHashListAsJson(const User& user, JsonString& json){
    std::string key = user.str();
    std::string value;

    leveldb::ReadOptions opts;
    leveldb::Status status;
    if(!(status = GetIndex()->Get(opts, key, &value)).ok()){
      LOG(WARNING) << "cannot get hash list for " << user << ": " << status.ToString();
      return false;
    }

    int64_t size = (int64_t) value.size();
    uint8_t* bytes = (uint8_t*) value.data();

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

  bool ObjectPool::GetHashList(const User& user, HashList& hashes){
    std::string key = user.str();
    std::string value;

    leveldb::ReadOptions opts;
    leveldb::Status status;
    if(!(status = GetIndex()->Get(opts, key, &value)).ok()){
      LOG(WARNING) << "cannot get hash list for " << user << ": " << status.ToString();
      return false;
    }

    return Decode((uint8_t*) value.data(), value.size(), hashes);
  }

  bool ObjectPool::PutObject(const Hash& hash, const UnclaimedTransactionPtr& val){
    if(HasObject(hash)){
      LOG(WARNING) << "cannot overwrite existing object pool data for: " << hash;
      SetStatus(ObjectPool::kWarning);
      return false;
    }

    ObjectPoolKey pkey(ObjectTag::kBlock, hash);
    BufferPtr kdata = Buffer::NewInstance(ObjectPoolKey::kSize);
    pkey.Write(kdata);

    int64_t vsize = val->GetBufferSize();
    BufferPtr vdata = Buffer::NewInstance(vsize);
    val->Write(vdata);

    leveldb::Slice key(kdata->data(), ObjectPoolKey::kSize);
    leveldb::Slice value(vdata->data(), vsize);

    leveldb::WriteOptions opts;
    opts.sync = true;
    leveldb::Status status;
    if(!(status = GetIndex()->Put(opts, key, value)).ok()){
      LOG(WARNING) << "cannot index object " << hash << ": " << status.ToString();
      return false;
    }

    //TODO: should we signal
    LOG(INFO) << "indexed object: " << hash;
    return true;
  }

  bool ObjectPool::GetBlocks(HashList& hashes){
    leveldb::Iterator* it = GetIndex()->NewIterator(leveldb::ReadOptions());
    for(it->SeekToFirst(); it->Valid(); it->Next()){
      ObjectPoolKey key(it->key());
      if(key.IsBlock())
        hashes.insert(key.GetHash());
    }
    return true;
  }

  bool ObjectPool::GetTransactions(HashList& hashes){
    leveldb::Iterator* it = GetIndex()->NewIterator(leveldb::ReadOptions());
    for(it->SeekToFirst(); it->Valid(); it->Next()){
      ObjectPoolKey key(it->key());
      if(key.IsTransaction())
        hashes.insert(key.GetHash());
    }
    return true;
  }

  bool ObjectPool::GetUnclaimedTransactions(HashList& hashes){
    leveldb::Iterator* it = GetIndex()->NewIterator(leveldb::ReadOptions());
    for(it->SeekToFirst(); it->Valid(); it->Next()){
      ObjectPoolKey key(it->key());
      if(key.IsUnclaimedTransaction())
        hashes.insert(key.GetHash());
    }
    return true;
  }

  bool ObjectPool::GetUnclaimedTransactionsFor(HashList& hashes, const User& user){
    leveldb::Iterator* it = GetIndex()->NewIterator(leveldb::ReadOptions());
    for(it->SeekToFirst(); it->Valid(); it->Next()){
      ObjectPoolKey key(it->key());
      if(!key.IsUnclaimedTransaction())
        continue;

      BufferPtr val = Buffer::From(it->value());
      UnclaimedTransactionPtr value = UnclaimedTransaction::NewInstance(val);
      LOG(INFO) << value->GetUser() << ": " << value->GetHash();
      if(value->GetUser() == user)
        hashes.insert(key.GetHash());
    }
    return true;
  }

  bool ObjectPool::VisitBlocks(ObjectPoolBlockVisitor* vis){
    if(!vis)
      return false;

    return false;
  }

  bool ObjectPool::VisitTransactions(ObjectPoolTransactionVisitor* vis){
    if(!vis)
      return false;
    return false;
  }

  bool ObjectPool::VisitUnclaimedTransactions(ObjectPoolUnclaimedTransactionVisitor* vis){
    if(!vis)
      return false;
    return false;
  }

  BlockPtr ObjectPool::GetBlock(const Hash& hash){
    ObjectPoolKey pkey(ObjectTag::kBlock, hash);
    BufferPtr kdata = Buffer::NewInstance(ObjectPoolKey::kSize);
    pkey.Write(kdata);

    leveldb::Slice key(kdata->data(), ObjectPoolKey::kSize);
    std::string value;

    leveldb::ReadOptions opts;
    leveldb::Status status;
    if(!(status = GetIndex()->Get(opts, key, &value)).ok()){
      LOG(WARNING) << "cannot get " << hash << ": " << status.ToString();
      return BlockPtr(nullptr);
    }

    BufferPtr buff = Buffer::From(value);
    return Block::NewInstance(buff);
  }

  TransactionPtr ObjectPool::GetTransaction(const Hash& hash){
    ObjectPoolKey pkey(ObjectTag::kTransaction, hash);
    BufferPtr kdata = Buffer::NewInstance(ObjectPoolKey::kSize);
    pkey.Write(kdata);

    leveldb::Slice key(kdata->data(), ObjectPoolKey::kSize);
    std::string value;

    leveldb::ReadOptions opts;
    leveldb::Status status;
    if(!(status = GetIndex()->Get(opts, key, &value)).ok()){
      LOG(WARNING) << "cannot get " << hash << ": " << status.ToString();
      return TransactionPtr(nullptr);
    }

    BufferPtr buff = Buffer::From(value);
    return Transaction::NewInstance(buff);
  }

  UnclaimedTransactionPtr ObjectPool::GetUnclaimedTransaction(const Hash& hash){
    ObjectPoolKey pkey(ObjectTag::kUnclaimedTransaction, hash);
    BufferPtr kdata = Buffer::NewInstance(ObjectPoolKey::kSize);
    pkey.Write(kdata);

    leveldb::Slice key(kdata->data(), ObjectTag::kUnclaimedTransaction);
    std::string value;

    leveldb::ReadOptions opts;
    leveldb::Status status;
    if(!(status = GetIndex()->Get(opts, key, &value)).ok()){
      LOG(WARNING) << "cannot get " << hash << ": " << status.ToString();
      return UnclaimedTransactionPtr(nullptr);
    }

    BufferPtr buff = Buffer::From(value);
    return UnclaimedTransaction::NewInstance(buff);
  }

  leveldb::Status ObjectPool::Write(leveldb::WriteBatch* update){
    leveldb::WriteOptions opts;
    opts.sync = true;
    return GetIndex()->Write(opts, update);
  }

  int64_t ObjectPool::GetNumberOfObjects(){
    int64_t count = 0;

    DIR* dir;
    struct dirent* ent;
    if((dir = opendir(GetDataDirectory().c_str())) != NULL){
      while((ent = readdir(dir)) != NULL){
        std::string name(ent->d_name);
        std::string filename = (GetDataDirectory() + "/" + name);
        if(!EndsWith(filename, ".dat"))
          continue;

        //TODO: fixme
      }
      closedir(dir);
    }
    return count;
  }

  int64_t ObjectPool::GetNumberOfObjectsByType(const ObjectTag::Type& type){
    int64_t count = 0;
    ObjectTag tag(type);
    leveldb::Iterator* it = GetIndex()->NewIterator(leveldb::ReadOptions());
    for(it->SeekToFirst(); it->Valid(); it->Next()){
      ObjectPoolKey key(it->key());
      if(key.GetTag() == tag)
        count++;
    }
    delete it;
    return count;
  }
}