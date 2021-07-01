#include "buffer.h"
#include "wallet_manager.h"

namespace token{
  //TODO: cleanup
  bool WalletManager::PutWallet(const User& user, const Wallet& wallet) const{
    BufferPtr buffer = Buffer::NewInstance(GetBufferSize(wallet));
    if(!Encode(buffer, wallet))
      return false;

    leveldb::WriteOptions options;
    options.sync = true;

    leveldb::Status status;
    if(!(status = GetIndex()->Put(options, (const leveldb::Slice&)user, buffer->operator leveldb::Slice())).ok()){
      LOG(WARNING) << "couldn't index wallet for user " << user << ": " << status.ToString();
      return false;
    }

    LOG(INFO) << "indexed wallet for user: " << user;
    return true;
  }

  Wallet WalletManager::GetUserWallet(const User& user) const{
    std::string data;
    leveldb::Status status;
    if(!(status = GetIndex()->Get(leveldb::ReadOptions(), (const leveldb::Slice&)user, &data)).ok()){
      LOG(WARNING) << "cannot get wallet for user " << user << ": " << status.ToString();
      return Wallet();
    }
    Wallet wallet;
    if(!Decode(data, wallet)){
      LOG(WARNING) << "cannot decode wallet for " << user;
      return Wallet();
    }
    return wallet;
  }

  //TODO: refactor
  bool WalletManager::GetWallet(const User& user, Wallet& wallet) const{
    std::string data;
    leveldb::Status status;
    if(!(status = GetIndex()->Get(leveldb::ReadOptions(), (const leveldb::Slice&)user, &data)).ok()){
      LOG(WARNING) << "cannot get wallet for user " << user << ": " << status.ToString();
      return false;
    }
    return Decode(data, wallet);
  }

//  //TODO: refactor
//  bool WalletManager::GetWallet(const User& user, Json::Writer& writer) const{
//    UserKey key(user);
//    std::string data;
//    leveldb::Status status;
//    if(!(status = GetIndex()->Get(leveldb::ReadOptions(), KEY(key), &data)).ok()){
//      LOG(WARNING) << "cannot get wallet for user " << user << ": " << status.ToString();
//      return false;
//    }
//
//    auto size = (int64_t) data.size();
//    auto bytes = (uint8_t*) data.data();
//
//    writer.StartArray();
//    int64_t offset = sizeof(int64_t);
//    while(offset < size){
//      Hash hash(&bytes[offset], Hash::kSize);
//      std::string hex = hash.HexString();
//      writer.String(hex.data(), 64);
//      offset += Hash::kSize;
//    }
//    writer.EndArray();
//    return true;
//  }

  //TODO: refactor
  bool WalletManager::RemoveWallet(const User& user) const{
    leveldb::WriteOptions options;
    options.sync = true;

    leveldb::Status status;
    if(!(status = GetIndex()->Delete(options, (const leveldb::Slice&)user)).ok()){
      LOG(WARNING) << "couldn't remove wallet for user " << user << ": " << status.ToString();
      return false;
    }

    LOG(INFO) << "removed wallet for user: " << user;
    return true;
  }

  bool WalletManager::HasWallet(const User& user) const{
    std::string data;
    return GetIndex()->Get(leveldb::ReadOptions(), (const leveldb::Slice&)user, &data).ok();
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

  WalletManagerPtr WalletManager::GetInstance(){
    static WalletManagerPtr instance = WalletManager::NewInstance();
    return instance;
  }

  bool WalletManager::Initialize(const std::string& filename){
    leveldb::Status status;
    if(!(status = GetInstance()->InitializeIndex<WalletManager::Comparator>(filename)).ok()){
      DLOG_WALLETS(ERROR) << "cannot initialize the wallet manager: " << status.ToString();
      return false;
    }
    return true;
  }
}