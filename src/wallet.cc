#include <mutex>
#include <condition_variable>
#include "wallet.h"
#include "utils/kvstore.h"

namespace Token{
  void ToJson(const Wallet& hashes, JsonString& sb){
    JsonWriter writer(sb);
    writer.StartArray();
    for(auto& it : hashes){
      std::string hash = it.HexString();
      writer.String(hash.data(), Hash::GetSize());
    }
    writer.EndArray();
  }

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
      LOG(WARNING) << "cannot re-initialize the wallet manager.";
      return false;
    }

    LOG(INFO) << "initializing the wallet manager....";
    SetState(WalletManager::kInitializing);

    leveldb::Options options;
    options.create_if_missing = true;
    options.comparator = new UserKey::Comparator();
    leveldb::Status status;
    if(!((status = leveldb::DB::Open(options, GetIndexFilename(), &index_)).ok())){
      LOG(WARNING) << "couldn't initialize the wallet manager index: " << status.ToString();
      return false;
    }

    LOG(INFO) << "the wallet manager has been initialized.";
    SetState(WalletManager::kInitialized);
    return true;
  }

  bool WalletManager::PutWallet(const User& user, const Wallet& wallet){
    leveldb::Status status;
    if(!(status = PutObject(GetIndex(), user, wallet)).ok()){
      LOG(WARNING) << "couldn't index wallet for user " << user << ": " << status.ToString();
      return false;
    }

    LOG(INFO) << "indexed wallet for user: " << user;
    return true;
  }

  bool WalletManager::GetWallet(const User& user, Wallet& wallet){
    std::string data;
    leveldb::Status status;
    if(!(status = GetObject(GetIndex(), UserKey(user), &data)).ok()){
      LOG(WARNING) << "cannot get wallet for user " << user << ": " << status.ToString();
      return false;
    }
    return Decode(data, wallet);
  }

  bool WalletManager::GetWallet(const User& user, JsonString& json){
    std::string data;

    leveldb::Status status;
    if(!(status = GetObject(GetIndex(), UserKey(user), &data)).ok()){
      LOG(WARNING) << "cannot get wallet for user " << user << ": " << status.ToString();
      return false;
    }

    int64_t size = (int64_t) data.size();
    uint8_t* bytes = (uint8_t*) data.data();

    JsonWriter writer(json);
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

  bool WalletManager::RemoveWallet(const User& user){
    leveldb::Status status;
    if(!(status = RemoveObject(GetIndex(), user)).ok()){
      LOG(WARNING) << "couldn't remove wallet for user " << user << ": " << status.ToString();
      return false;
    }

    LOG(INFO) << "removed wallet for user: " << user;
    return true;
  }

  bool WalletManager::HasWallet(const User& user){
    return HasObject(GetIndex(), user);
  }

  leveldb::Status WalletManager::Write(leveldb::WriteBatch* batch){
    leveldb::WriteOptions opts;
    opts.sync = true;
    return GetIndex()->Write(opts, batch);
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