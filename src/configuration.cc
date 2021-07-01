#include <leveldb/db.h>
#include <leveldb/status.h>

#include "buffer.h"
#include "configuration.h"

namespace token{
#define CANNOT_GET_PROPERTY(PropertyName) \
  DLOG_CONFIG(WARNING) << "cannot get property: " << (PropertyName)
#define CANNOT_SET_PROPERTY(PropertyName, PropertyValue) \
  DLOG_CONFIG(WARNING) << "cannot set property " << (PropertyName) << " to " << (PropertyValue)
#define SET_PROPERTY(PropertyName, PropertyValue) \
  DLOG_CONFIG(INFO) << "set property " << (PropertyName) << " to " << (PropertyValue)
#define ERROR_SETTING_PROPERTY(PropertyName, Status) \
  LOG_CONFIG(ERROR) << "error setting property " << (PropertyName) << ": " << status.ToString()
#define ERROR_GETTING_PROPERTY(PropertyName, Status) \
  LOG_CONFIG(ERROR) << "error getting property " << (PropertyName) << ": " << status.ToString()

  static inline void
  PutDefaultProperty(leveldb::WriteBatch& batch, const std::string& name, const UUID& val){
    batch.Put(name, leveldb::Slice(val.data(), val.size()));
  }

  static inline void
  PutDefaultProperty(leveldb::WriteBatch& batch, const std::string& name, const std::string& val){
    batch.Put(name, val);
  }

  static inline void
  PutDefaultProperty(leveldb::WriteBatch& batch, const std::string& name, const PeerList& val){
    BufferPtr buffer = Buffer::NewInstance(GetBufferSize(val));
    if(!buffer->PutPeerList(val)){
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
      DLOG_CONFIG_IF(WARNING, !NodeAddress::ResolveAddresses(FLAGS_remote, peers)) << "couldn't resolve remote address: " << FLAGS_remote;
    }
    PutDefaultProperty(batch, TOKEN_CONFIGURATION_NODE_PEERS, peers);

    leveldb::WriteOptions options;
    options.sync = true;
    return GetIndex()->Write(options, &batch);
  }

  leveldb::Status ConfigurationManager::InitializeIndex(const std::string& filename){
    if(!IsUninitializedState())
      return leveldb::Status::NotSupported("Cannot re-initialize the ConfigurationManager.");

    DLOG_CONFIG(INFO) << "initializing in " << filename << "....";
    SetState(ConfigurationManager::kInitializingState);
    bool first_initialization = !FileExists(filename);

    leveldb::Options options;
    options.create_if_missing = true;

    leveldb::Status status;
    if(!(status = leveldb::DB::Open(options, filename, &index_)).ok()){
      LOG_CONFIG(ERROR) << "couldn't initialize the index: " << status.ToString();
      return status;
    }

    if(first_initialization){
      if(!(status = SetDefaults(filename)).ok()){
        LOG_CONFIG(ERROR) << "cannot set defaults: " << status.ToString();
        return status;
      }
    }

    SetState(ConfigurationManager::kInitializedState);
    DLOG_CONFIG(INFO) << "initialized.";
    return leveldb::Status::OK();
  }

  bool ConfigurationManager::HasProperty(const std::string& name) const{
    std::string value;

    leveldb::Status status;
    if(!(status = GetIndex()->Get(leveldb::ReadOptions(), name, &value)).ok()){
      if(status.IsNotFound()){
        CANNOT_GET_PROPERTY(name);
        return false;
      }

      ERROR_GETTING_PROPERTY(name, status);
      return false;
    }
    return true;
  }

  //TODO: refactor?
  bool ConfigurationManager::PutProperty(const std::string& name, const PeerList& val) const{
    leveldb::WriteOptions options;
    options.sync = true;

    BufferPtr buffer = Buffer::NewInstance(GetBufferSize(val));
    if(!buffer->PutPeerList(val)){
      LOG(WARNING) << "cannot encode peer list to buffer of size " << buffer->GetBufferSize();
      return false;
    }

    leveldb::Slice value = buffer->operator leveldb::Slice();
    leveldb::Status status;
    if(!(status = GetIndex()->Put(options, name, value)).ok()){
      ERROR_SETTING_PROPERTY(name, status);
      return false;
    }

    SET_PROPERTY(name, val);
    return true;
  }

  bool ConfigurationManager::PutProperty(const std::string& name, const UUID& value) const{
    leveldb::WriteOptions options;
    options.sync = true;

    leveldb::Status status;
    if(!(status = GetIndex()->Put(options, name, (const leveldb::Slice&)value)).ok()){
      ERROR_SETTING_PROPERTY(name, status);
      return false;
    }

    SET_PROPERTY(name, value);
    return true;
  }

  bool ConfigurationManager::PutProperty(const std::string& name, const std::string& val) const{
    leveldb::WriteOptions options;
    options.sync = true;

    leveldb::Status status;
    if(!(status = GetIndex()->Put(options, name, val)).ok()){
      ERROR_SETTING_PROPERTY(name, val);
      return false;
    }

    SET_PROPERTY(name, val);
    return true;
  }

  bool ConfigurationManager::PutProperty(const std::string& name, const NodeAddress& val) const{
    leveldb::WriteOptions options;
    options.sync = true;

    leveldb::Status status;
    if(!(status = GetIndex()->Put(options, name, (const leveldb::Slice&)val)).ok()){
      ERROR_SETTING_PROPERTY(name, val);
      return false;
    }

    SET_PROPERTY(name, val);
    return true;
  }

  bool ConfigurationManager::PutProperty(const std::string& name, const Hash& val) const{
    leveldb::WriteOptions options;
    options.sync = true;

    leveldb::Status status;
    if(!(status = GetIndex()->Put(options, name, (const leveldb::Slice&)val)).ok()){
      ERROR_SETTING_PROPERTY(name, val);
      return false;
    }
    SET_PROPERTY(name, val);
    return true;
  }

  bool ConfigurationManager::GetString(const std::string& name, std::string& value) const{
    leveldb::Status status;
    if(!(status = GetIndex()->Get(leveldb::ReadOptions(), name, &value)).ok()){
      if(status.IsNotFound()){
        CANNOT_GET_PROPERTY(name);
        return false;
      }

      ERROR_GETTING_PROPERTY(name, status);
      return false;
    }
    return true;
  }

  bool ConfigurationManager::GetHash(const std::string& name, Hash& result) const{
    std::string value;

    leveldb::Status status;
    if(!(status = GetIndex()->Get(leveldb::ReadOptions(), name, &value)).ok()){
      if(status.IsNotFound()){
        CANNOT_GET_PROPERTY(name);
        return false;
      }

      ERROR_GETTING_PROPERTY(name, status);
      return false;
    }

    result = Hash::FromHexString(value);
    return true;
  }

  bool ConfigurationManager::GetUUID(const std::string& name, UUID& val) const{
    ConfigurationManager* cfg_mngr = ConfigurationManager::GetInstance();
    if(!cfg_mngr->IsInitializedState()){
      val = UUID();
      return true;//TODO: fixme
    }

    std::string value;

    leveldb::Status status;
    if(!(status = GetIndex()->Get(leveldb::ReadOptions(), name, &value)).ok()){
      if(status.IsNotFound()){
        CANNOT_GET_PROPERTY(name);
        return false;
      }

      ERROR_SETTING_PROPERTY(name, status);
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
        CANNOT_GET_PROPERTY(name);
        return false;
      }

      ERROR_GETTING_PROPERTY(name, status);
      return false;
    }

    BufferPtr buffer = Buffer::From(value);
    return buffer->GetPeerList(peers);
  }

  static ConfigurationManager instance;

  bool ConfigurationManager::Initialize(const std::string& filename){
    leveldb::Status status;
    if(!(status = instance.InitializeIndex(filename)).ok()){
      LOG_CONFIG(ERROR) << "couldn't initialize the configuration in " << filename << ": " << status.ToString();
      return false;
    }
    return true;
  }

  ConfigurationManager* ConfigurationManager::GetInstance(){
    return &instance;
  }
}