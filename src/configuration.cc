#include <leveldb/db.h>
#include <leveldb/status.h>
#include "configuration.h"
#include "atomic/relaxed_atomic.h"

namespace token{
  static RelaxedAtomic<ConfigurationManager::State> state_ = { ConfigurationManager::kUninitializedState };
  static leveldb::DB* index_ = nullptr;

  static inline leveldb::DB*
  GetIndex(){
    return index_;
  }

  static inline std::string
  GetIndexFilename(){
    return TOKEN_BLOCKCHAIN_HOME + "/config";
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
  PutDefaultProperty(leveldb::WriteBatch& batch, const std::string& name, const PeerList& val){
    BufferPtr buffer = Buffer::NewInstance(GetBufferSize(val));
    if(!Encode(buffer, val)){
      LOG(WARNING) << "cannot serialize peer list to buffer of size " << buffer->GetBufferSize();
      return;
    }
    batch.Put(name, buffer->operator leveldb::Slice());
  }

  leveldb::Status ConfigurationManager::SetDefaults(){
    leveldb::WriteBatch batch;


    LOG(INFO) << "setting configuration manager defaults....";
    PutDefaultProperty(batch, TOKEN_CONFIGURATION_NODE_ID, UUID());

    PeerList peers;
    if(!FLAGS_remote.empty()){
      if(!NodeAddress::ResolveAddresses(FLAGS_remote, peers))
        LOG(WARNING) << "cannot resolve peer address for: " << FLAGS_remote;
    }
    PutDefaultProperty(batch, TOKEN_CONFIGURATION_NODE_PEERS, peers);

    leveldb::WriteOptions options;
    options.sync = true;
    return GetIndex()->Write(options, &batch);
  }

  bool ConfigurationManager::Initialize(){
    if(!IsUninitializedState()){
      LOG(WARNING) << "cannot re-initialize the configuration manager.";
      return false;
    }

    LOG(INFO) << "initializing the configuration manager....";
    SetState(ConfigurationManager::kInitializingState);

    bool first_initialization = !FileExists(GetIndexFilename());

    leveldb::Options options;
    options.create_if_missing = true;

    leveldb::Status status;
    if(!(status = leveldb::DB::Open(options, GetIndexFilename(), &index_)).ok()){
      LOG(WARNING) << "couldn't initialize the configuration index " << GetIndexFilename() << ": " << status.ToString();
      return false;
    }

    if(first_initialization){
      if(!(status = SetDefaults()).ok())
        LOG(WARNING) << "cannot set configuration manager defaults: " << status.ToString();
    }

    LOG(INFO) << "configuration manager initialized.";
    SetState(ConfigurationManager::kInitializedState);
    return true;
  }

  bool ConfigurationManager::HasProperty(const std::string& name){
    std::string value;

    leveldb::Status status;
    if(!(status = GetIndex()->Get(leveldb::ReadOptions(), name, &value)).ok()){
      if(status.IsNotFound())
        return false;
      LOG(WARNING) << "error getting configuration property " << name << ": " << status.ToString();
      return false;
    }
    return true;
  }

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
      LOG(WARNING) << "cannot put configuration property " << name << ": " << status.ToString();
      return false;
    }

#ifdef TOKEN_DEBUG
    LOG(INFO) << "set configuration property " << name << " to " << val;
#endif//TOKEN_DEBUG
    return true;
  }

  UUID ConfigurationManager::GetID(const std::string& name){
    std::string value;

    leveldb::Status status;
    if(!(status = GetIndex()->Get(leveldb::ReadOptions(), name, &value)).ok()){
      if(status.IsNotFound()){
        LOG(WARNING) << "cannot find configuration property " << name;
        return UUID();
      }

      LOG(WARNING) << "error getting configuration property " << name << ": " << status.ToString();
      return UUID();
    }
    return UUID(value);
  }

  bool ConfigurationManager::GetPeerList(const std::string& name, PeerList& peers){
    std::string value;

    leveldb::Status status;
    if(!(status = GetIndex()->Get(leveldb::ReadOptions(), name, &value)).ok()){
      if(status.IsNotFound()){
        LOG(WARNING) << "cannot find configuration property " << name;
        return false;
      }

      LOG(WARNING) << "error getting configuration property " << name << ": " << status.ToString();
      return false;
    }

    BufferPtr buffer = Buffer::From(value);
    return Decode(buffer, peers);
  }
}