#include <leveldb/db.h>
#include <leveldb/status.h>
#include "configuration.h"
#include "atomic/relaxed_atomic.h"

namespace token{
#define CONFIG_LOG(LevelName) \
  LOG(LevelName) << "[ConfigManager] "

#ifdef TOKEN_DEBUG
  #define CANNOT_GET_PROPERTY(LevelName, PropertyName) \
    CONFIG_LOG(LevelName) << "cannot get property '" << (PropertyName) << "'"

  #define CANNOT_SET_PROPERTY(LevelName, PropertyName, PropertyValue) \
    CONFIG_LOG(LevelName) << "cannot set property " << (PropertyName) << " to: " << (PropertyValue)

  #define SET_PROPERTY(LevelName, PropertyName, PropertyValue) \
    CONFIG_LOG(LevelName) << "set property " << (PropertyName) << " to: " << (PropertyValue)
#else
  #define CANNOT_GET_PROPERTY(LevelName, PropertyName){}

  #define CNANOT_SET_PROPERTY(LevelName, PropertyName, PropertyValue){}

  #define SET_PROPERTY(LevelName, PropertyName, PropertyValue) {}
#endif//TOKEN_DEBUG

#define ERROR_SETTING_PROPERTY(LevelName, PropertyName, Status) \
  CONFIG_LOG(LevelName) << "error setting property " << (PropertyName) << ": " << status.ToString()

#define ERROR_GETTING_PROPERTY(LevelName, PropertyName, Status) \
  CONFIG_LOG(LevelName) << "error getting property " << (PropertyName) << ": " << status.ToString()

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

  leveldb::Status ConfigurationManager::InitializeIndex(const std::string& parent){
    if(!IsUninitializedState()){
#ifdef TOKEN_DEBUG
      CONFIG_LOG(WARNING) << "cannot re-initialize the configuration manager.";
#endif//TOKEN_DEBUG
      return leveldb::Status::NotSupported("Cannot re-initialize the ConfigurationManager.");
    }

    std::string filename = parent + "/config";
#ifdef TOKEN_DEBUG
    CONFIG_LOG(INFO) << "initializing the ConfigurationManager in " << filename << "....";
#endif//TOKEN_DEBUG
    SetState(ConfigurationManager::kInitializingState);
    bool first_initialization = !FileExists(filename);

    leveldb::Options options;
    options.create_if_missing = true;

    leveldb::Status status;
    if(!(status = leveldb::DB::Open(options, filename, &index_)).ok()){
      CONFIG_LOG(ERROR) << "couldn't initialize the index: " << status.ToString();
      return status;
    }

    if(first_initialization){
      if(!(status = SetDefaults(filename)).ok()){
        CONFIG_LOG(ERROR) << "cannot set defaults: " << status.ToString();
        return status;
      }
    }

    SetState(ConfigurationManager::kInitializedState);
#ifdef TOKEN_DEBUG
    CONFIG_LOG(INFO) << "initialized.";
#endif//TOKEN_DEBUG
    return leveldb::Status::OK();
  }

  bool ConfigurationManager::HasProperty(const std::string& name) const{
    std::string value;

    leveldb::Status status;
    if(!(status = GetIndex()->Get(leveldb::ReadOptions(), name, &value)).ok()){
      if(status.IsNotFound()){
        CANNOT_GET_PROPERTY(WARNING, name);
        return false;
      }

      ERROR_GETTING_PROPERTY(ERROR, name, status);
      return false;
    }
    return true;
  }

  //TODO: refactor?
  bool ConfigurationManager::PutProperty(const std::string& name, const PeerList& val) const{
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
      ERROR_SETTING_PROPERTY(ERROR, name, status);
      return false;
    }

    SET_PROPERTY(INFO, name, val);
    return true;
  }

  bool ConfigurationManager::PutProperty(const std::string& name, const UUID& value) const{
    leveldb::WriteOptions options;
    options.sync = true;

    leveldb::Status status;
    if(!(status = GetIndex()->Put(options, name, value)).ok()){
      ERROR_SETTING_PROPERTY(ERROR, name, status);
      return false;
    }

    SET_PROPERTY(INFO, name, value);
    return true;
  }

  bool ConfigurationManager::PutProperty(const std::string& name, const std::string& val) const{
    leveldb::WriteOptions options;
    options.sync = true;

    leveldb::Status status;
    if(!(status = GetIndex()->Put(options, name, val)).ok()){
      ERROR_SETTING_PROPERTY(ERROR, name, val);
      return false;
    }

    SET_PROPERTY(INFO, name, val);
    return true;
  }

  bool ConfigurationManager::PutProperty(const std::string& name, const NodeAddress& val) const{
    leveldb::WriteOptions options;
    options.sync = true;

    leveldb::Status status;
    if(!(status = GetIndex()->Put(options, name, val)).ok()){
      ERROR_SETTING_PROPERTY(ERROR, name, val);
      return false;
    }

    SET_PROPERTY(INFO, name, val);
    return true;
  }

  bool ConfigurationManager::GetString(const std::string& name, std::string& value) const{
    leveldb::Status status;
    if(!(status = GetIndex()->Get(leveldb::ReadOptions(), name, &value)).ok()){
      if(status.IsNotFound()){
        CANNOT_GET_PROPERTY(WARNING, name);
        return false;
      }

      ERROR_GETTING_PROPERTY(ERROR, name, status);
      return false;
    }
    return true;
  }

  bool ConfigurationManager::GetUUID(const std::string& name, UUID& val) const{
    std::string value;

    leveldb::Status status;
    if(!(status = GetIndex()->Get(leveldb::ReadOptions(), name, &value)).ok()){
      if(status.IsNotFound()){
        CANNOT_GET_PROPERTY(WARNING, name);
        return false;
      }

      ERROR_SETTING_PROPERTY(ERROR, name, status);
      return false;
    }

    val = UUID(value);
    return true;
  }

  bool ConfigurationManager::GetPeerList(const std::string& name, PeerList& peers) const{
    std::string value;

    leveldb::Status status;
    if(!(status = GetIndex()->Get(leveldb::ReadOptions(), name, &value)).ok()){
      if(status.IsNotFound()){
        CANNOT_GET_PROPERTY(WARNING, name);
        return false;
      }

      ERROR_GETTING_PROPERTY(ERROR, name, status);
      return false;
    }

    BufferPtr buffer = Buffer::From(value);
    return Decode(buffer, peers);
  }

  static ConfigurationManager instance;

  bool ConfigurationManager::Initialize(const std::string& filename){
#ifdef TOKEN_DEBUG
    CONFIG_LOG(INFO) << "initializing the ConfigurationManager in " << filename << "....";
#endif//TOKEN_DEBUG

    leveldb::Status status;
    if(!(status = instance.InitializeIndex(filename)).ok()){
      CONFIG_LOG(ERROR) << "couldn't initialize the ConfigurationManager in " << filename << ": " << status.ToString();
      return false;
    }
    return true;
  }

  ConfigurationManager* ConfigurationManager::GetInstance(){
    return &instance;
  }
}