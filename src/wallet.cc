#include <mutex>
#include <condition_variable>

#include <leveldb/db.h>
#include <leveldb/slice.h>

#include "block.h"
#include "wallet.h"

namespace token{
  void ToJson(const Wallet& hashes, Json::String& sb){
    Json::Writer writer(sb);
    writer.StartArray();
    for(auto& it : hashes){
      std::string hash = it.HexString();
      writer.String(hash.data(), Hash::GetSize());
    }
    writer.EndArray();
  }

#define WMGR_LOG(LevelName) \
  LOG(LevelName) << "[WalletManager] "


  //TODO: remove locking
  static std::mutex mutex_;
  static std::condition_variable cond_;
  static WalletManager::State state_ = WalletManager::kUninitialized;
  static WalletManager::Status status_ = WalletManager::kOk;
  static leveldb::DB* index_ = nullptr;

  static inline leveldb::DB*
  GetIndex(){
    return index_;
  }

  static inline std::string
  GetIndexFilename(){
    return TOKEN_BLOCKCHAIN_HOME + "/wallet";
  }

  WalletManager::State WalletManager::GetState(){
    std::lock_guard<std::mutex> guard(mutex_);
    return state_;
  }

  void WalletManager::SetState(const State& state){
    std::unique_lock<std::mutex> lock(mutex_);
    state_ = state;
    lock.unlock();
    cond_.notify_all();
  }

  WalletManager::Status WalletManager::GetStatus(){
    std::lock_guard<std::mutex> guard(mutex_);
    return status_;
  }

  void WalletManager::SetStatus(const Status& status){
    std::unique_lock<std::mutex> lock(mutex_);
    status_ = status;
    lock.unlock();
    cond_.notify_all();
  }

  bool WalletManager::Initialize(){
    if(IsInitialized()){
#ifdef TOKEN_DEBUG
      WMGR_LOG(WARNING) << "cannot re-initialize the wallet manager.";
#endif//TOKEN_DEBUG
      return false;
    }

    WMGR_LOG(INFO) << "initializing....";
    SetState(WalletManager::kInitializing);

    leveldb::Options options;
    options.create_if_missing = true;
    options.comparator = new WalletManager::Comparator();
    leveldb::Status status;
    if(!((status = leveldb::DB::Open(options, GetIndexFilename(), &index_)).ok())){
      WMGR_LOG(ERROR) << "couldn't initialize the index: " << status.ToString();
      return false;
    }

#ifdef TOKEN_DEBUG
    WMGR_LOG(INFO) << "initialized.";
#endif//TOKEN_DEBUG
    SetState(WalletManager::kInitialized);
    return true;
  }

  //TODO: cleanup
  bool WalletManager::PutWallet(const User& user, const Wallet& wallet){
    UserKey key(user);

    BufferPtr buffer = Buffer::NewInstance(GetBufferSize(wallet));
    if(!Encode(wallet, buffer))
      return false;

    leveldb::WriteOptions options;
    options.sync = true;

    leveldb::Status status;
    if(!(status = GetIndex()->Put(options, key, buffer->operator leveldb::Slice())).ok()){
      LOG(WARNING) << "couldn't index wallet for user " << user << ": " << status.ToString();
      return false;
    }

    LOG(INFO) << "indexed wallet for user: " << user;
    return true;
  }

  //TODO: refactor
  bool WalletManager::GetWallet(const User& user, Wallet& wallet){
    UserKey key(user);

    std::string data;
    leveldb::Status status;
    if(!(status = GetIndex()->Get(leveldb::ReadOptions(), key, &data)).ok()){
      LOG(WARNING) << "cannot get wallet for user " << user << ": " << status.ToString();
      return false;
    }
    return Decode(data, wallet);
  }

  //TODO: refactor
  bool WalletManager::GetWallet(const User& user, Json::Writer& writer){
    UserKey key(user);
    std::string data;
    leveldb::Status status;
    if(!(status = GetIndex()->Get(leveldb::ReadOptions(), key, &data)).ok()){
      LOG(WARNING) << "cannot get wallet for user " << user << ": " << status.ToString();
      return false;
    }

    int64_t size = (int64_t) data.size();
    uint8_t* bytes = (uint8_t*) data.data();

    writer.StartArray();
    int64_t offset = sizeof(int64_t);
    while(offset < size){
      Hash hash(&bytes[offset], Hash::kSize);
      std::string hex = hash.HexString();
      writer.String(hex.data(), 64);
      offset += Hash::kSize;
    }
    writer.EndArray();
    return true;
  }

  //TODO: refactor
  bool WalletManager::RemoveWallet(const User& user){
    leveldb::WriteOptions options;
    options.sync = true;

    UserKey key(user);

    leveldb::Status status;
    if(!(status = GetIndex()->Delete(options, key)).ok()){
      LOG(WARNING) << "couldn't remove wallet for user " << user << ": " << status.ToString();
      return false;
    }

    LOG(INFO) << "removed wallet for user: " << user;
    return true;
  }

  bool WalletManager::HasWallet(const User& user){
    std::string data;
    UserKey key(user);
    return GetIndex()->Get(leveldb::ReadOptions(), key, &data).ok();
  }

  leveldb::Status WalletManager::Commit(const leveldb::WriteBatch& batch){
    leveldb::WriteOptions opts;
    opts.sync = true;
    return GetIndex()->Write(opts, (leveldb::WriteBatch*)&batch);
  }

  int64_t WalletManager::GetNumberOfWallets(){
    int64_t count = 0;
    leveldb::Iterator* it = GetIndex()->NewIterator(leveldb::ReadOptions());
    for(it->SeekToFirst(); it->Valid(); it->Next())
      count++;
    delete it;
    return count;
  }
}