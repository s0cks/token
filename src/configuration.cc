#include <leveldb/db.h>
#include <leveldb/status.h>
#include "configuration.h"
#include "atomic/relaxed_atomic.h"

namespace token{
#define CONFIG_LOG(LevelName) \
  LOG(LevelName) << "[ConfigManager] "

  static RelaxedAtomic<ConfigurationManager::State> state_ = { ConfigurationManager::kUninitializedState };
  static leveldb::DB* index_ = nullptr;

  static inline leveldb::DB*
  GetIndex(){
    return index_;
  }

  void ConfigurationManager::SetState(const State& state){
    state_ = state;
  }

  ConfigurationManager::State ConfigurationManager::GetState(){
    return state_;
  }

  static inline void
  PutDefaultProperty(leveldb::WriteBatch& batch, const std::string& name, const UUID& val){
    batch.Put(name, val);
  }

  static inline void
  PutDefaultProperty(leveldb::WriteBatch& batch, const std::string& name, const std::string& val){
    batch.Put(name, val);
  }

  static inline void
  PutDefaultProperty(leveldb::WriteBatch& batch, const std::string& name, const PeerList& val){
    BufferPtr buffer = Buffer::NewInstance(GetBufferSize(val));
    if(!Encode(buffer, val)){
      LOG(WARNING) << "cannot serialize peer list to buffer of size " << buffer->GetBufferSize();
      return;
    }
    batch.Put(name, buffer->operator leveldb::Slice());
  }

  leveldb::Status ConfigurationManager::SetDefaults(const std::string& home_dir){
    leveldb::WriteBatch batch;
    PutDefaultProperty(batch, TOKEN_CONFIGURATION_NODE_ID, UUID());
    PutDefaultProperty(batch, TOKEN_CONFIGURATION_BLOCKCHAIN_HOME, home_dir);

    PeerList peers;
    if(!FLAGS_remote.empty()){
      if(!NodeAddress::ResolveAddresses(FLAGS_remote, peers))
        CONFIG_LOG(WARNING) << "cannot resolve remote address: " << FLAGS_remote;
    }
    PutDefaultProperty(batch, TOKEN_CONFIGURATION_NODE_PEERS, peers);

    leveldb::WriteOptions options;
    options.sync = true;
    return GetIndex()->Write(options, &batch);
  }

  bool ConfigurationManager::Initialize(const std::string& home_dir){
    if(!IsUninitializedState()){
#ifdef TOKEN_DEBUG
      CONFIG_LOG(WARNING) << "cannot re-initialize the configuration manager.";
#endif//TOKEN_DEBUG
      return false;
    }

    SetState(ConfigurationManager::kInitializingState);
    std::string config_directory = home_dir + "/config";
#ifdef TOKEN_DEBUG
    CONFIG_LOG(INFO) << "initializing in " << config_directory << "....";
#endif//TOKEN_DEBUG

    bool first_initialization = !FileExists(config_directory);

    leveldb::Options options;
    options.create_if_missing = true;

    leveldb::Status status;
    if(!(status = leveldb::DB::Open(options, config_directory, &index_)).ok()){
      CONFIG_LOG(ERROR) << "couldn't initialize the index: " << status.ToString();
      return false;
    }

    if(first_initialization){
      if(!(status = SetDefaults(home_dir)).ok()){
        CONFIG_LOG(ERROR) << "cannot set defaults: " << status.ToString();
        return false;
      }
    }

    SetState(ConfigurationManager::kInitializedState);
#ifdef TOKEN_DEBUG
    CONFIG_LOG(INFO) << "initialized.";
#endif//TOKEN_DEBUG
    return true;
  }

  bool ConfigurationManager::HasProperty(const std::string& name){
    std::string value;

    leveldb::Status status;
    if(!(status = GetIndex()->Get(leveldb::ReadOptions(), name, &value)).ok()){
      if(status.IsNotFound()){
#ifdef TOKEN_DEBUG
        CONFIG_LOG(WARNING) << "cannot find property '" << name << "'";
#endif//TOKEN_DEBUG
        return false;
      }

#ifdef TOKEN_DEBUG
      CONFIG_LOG(ERROR) << "cannot get property " << name << ": " << status.ToString();
#endif//TOKEN_DEBUG
      return false;
    }
    return true;
  }

  //TODO: refactor?
  bool ConfigurationManager::SetProperty(const std::string& name, const PeerList& val){
    leveldb::WriteOptions options;
    options.sync = true;

    BufferPtr buffer = Buffer::NewInstance(GetBufferSize(val));
    if(!Encode(buffer, val)){
      LOG(WARNING) << "cannot encode peer list to buffer of size " << buffer->GetBufferSize();
      return false;
    }

    leveldb::Slice value = buffer->operator leveldb::Slice();
    leveldb::Status status;
    if(!(status = GetIndex()->Put(options, name, value)).ok()){
      LOG(WARNING) << "cannot put configuration property " << name << ": " << status.ToString();
      return false;
    }

#ifdef TOKEN_DEBUG
    LOG(INFO) << "set configuration property " << name << " to " << val;
#endif//TOKEN_DEBUG
    return true;
  }

  bool ConfigurationManager::SetProperty(const std::string& name, const UUID& val){
    leveldb::WriteOptions options;
    options.sync = true;

    leveldb::Status status;
    if(!(status = GetIndex()->Put(options, name, val)).ok()){
#ifdef TOKEN_DEBUG
      CONFIG_LOG(ERROR) << "cannot set property " << name << ": " << status.ToString();
#endif//TOKEN_DEBUG
      return false;
    }

#ifdef TOKEN_DEBUG
    CONFIG_LOG(INFO) << "set configuration property '" << name << "' to: " << val;
#endif//TOKEN_DEBUG
    return true;
  }

  std::string ConfigurationManager::GetString(const std::string& name){
    std::string value;

    leveldb::Status status;
    if(!(status = GetIndex()->Get(leveldb::ReadOptions(), name, &value)).ok()){
      if(status.IsNotFound()){
#ifdef TOKEN_DEBUG
        CONFIG_LOG(WARNING) << "cannot find property '" << name << "'";
#endif//TOKEN_DEBUG
        return "";
      }

#ifdef TOKEN_DEBUG
      CONFIG_LOG(ERROR) << "error getting property " << name << ": " << status.ToString();
#endif//TOKEN_DEBUG
      return "";
    }
    return value;
  }

  UUID ConfigurationManager::GetID(const std::string& name){
    std::string value;

    leveldb::Status status;
    if(!(status = GetIndex()->Get(leveldb::ReadOptions(), name, &value)).ok()){
      if(status.IsNotFound()){
#ifdef TOKEN_DEBUG
        CONFIG_LOG(WARNING) << "cannot find property '" << name << "'";
#endif//TOKEN_DEBUG
        return UUID();
      }

#ifdef TOKEN_DEBUG
      CONFIG_LOG(ERROR) << "error getting property " << name << ": " << status.ToString();
#endif//TOKEN_DEBUG
      return UUID();
    }
    return UUID(value);
  }

  bool ConfigurationManager::GetPeerList(const std::string& name, PeerList& peers){
    std::string value;

    leveldb::Status status;
    if(!(status = GetIndex()->Get(leveldb::ReadOptions(), name, &value)).ok()){
      if(status.IsNotFound()){
#ifdef TOKEN_DEBUG
        CONFIG_LOG(WARNING) << "cannot find property '" << name << "'";
#endif//TOKEN_DEBUG
        return false;
      }

#ifdef TOKEN_DEBUG
      CONFIG_LOG(ERROR) << "error getting property " << name << ": " << status.ToString();
#endif//TOKEN_DEBUG
      return false;
    }

    BufferPtr buffer = Buffer::From(value);
    return Decode(buffer, peers);
  }
}