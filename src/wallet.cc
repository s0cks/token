#include <leveldb/db.h>
#include <leveldb/slice.h>

#include "wallet.h"
#include "configuration.h"

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

  static inline std::string
  GetBlockChainHome(){
    return ConfigurationManager::GetString(TOKEN_CONFIGURATION_BLOCKCHAIN_HOME);
  }

  static inline std::string
  GetIndexFilename(){
    return GetBlockChainHome() + "/wallet";
  }

  //TODO: cleanup
  bool WalletManager::PutWallet(const User& user, const Wallet& wallet) const{
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
  bool WalletManager::GetWallet(const User& user, Wallet& wallet) const{
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
  bool WalletManager::GetWallet(const User& user, Json::Writer& writer) const{
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
  bool WalletManager::RemoveWallet(const User& user) const{
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

  bool WalletManager::HasWallet(const User& user) const{
    std::string data;
    UserKey key(user);
    return GetIndex()->Get(leveldb::ReadOptions(), key, &data).ok();
  }

  leveldb::Status WalletManager::Commit(const leveldb::WriteBatch& batch){
    leveldb::WriteOptions opts;
    opts.sync = true;
    return GetIndex()->Write(opts, (leveldb::WriteBatch*)&batch);
  }

  int64_t WalletManager::GetNumberOfWallets() const{
    int64_t count = 0;
    leveldb::Iterator* it = GetIndex()->NewIterator(leveldb::ReadOptions());
    for(it->SeekToFirst(); it->Valid(); it->Next())
      count++;
    delete it;
    return count;
  }

  class LocalWalletManager : public WalletManager{
   private:
    bool Initialize(){
      if(IsInitialized()){
#ifdef TOKEN_DEBUG
        WMGR_LOG(WARNING) << "cannot re-initialize the wallet manager.";
#endif//TOKEN_DEBUG
        return false;
      }

      WMGR_LOG(INFO) << "initializing....";
      SetState(WalletManager::kInitializingState);

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
      SetState(WalletManager::kInitializedState);
      return true;
    }
   public:
    LocalWalletManager():
      WalletManager(){
      if(!Initialize())
        WMGR_LOG(ERROR) << "failed to initialize the wallet manager.";
    }
    ~LocalWalletManager() = default;
  };

  WalletManager* WalletManager::GetInstance(){
    static LocalWalletManager instance;
    return &instance;
  }
}