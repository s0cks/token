#include <leveldb/db.h>
#include <leveldb/status.h>

#include "buffer.h"
#include "config.h"

namespace token{
  Configuration::Configuration(const std::string& filename):
    state_(State::kUninitialized),
    filename_(filename),
    index_(nullptr){
    Initialize();
  }

  void Configuration::Initialize(){
    if(!IsUninitialized()){
      LOG(FATAL) << "cannot re-initialize the configuration.";
      return;
    }

    bool first_initialization = !FileExists(GetFilename());
    SetState(State::kInitializing);

    leveldb::Options options;
    options.create_if_missing = true;

    leveldb::Status status;
    if(!(status = leveldb::DB::Open(options, GetFilename(), &index_)).ok()){
      LOG(FATAL) << "couldn't load the configuration index from " << GetFilename() << ": " << status.ToString();
      return;
    }

    if(first_initialization){
      SetDefaults();
      DLOG(INFO) << "initialized the configuration in: " << GetFilename();
    } else{
      DLOG(INFO) << "loaded the configuration from: " << GetFilename();
    }
    SetState(State::kInitialized);
  }

#define DEFINE_SET_DEFAULT(Name, Type) \
    static inline void SetDefault(leveldb::WriteBatch& batch, const std::string& name, const Type& val){ return batch.Put(name, (leveldb::Slice)val); }
  FOR_EACH_CONFIG_PROPERTY_TYPE(DEFINE_SET_DEFAULT)
#undef DEFINE_SET_DEFAULT

  void Configuration::SetDefaults(){
    leveldb::WriteBatch batch;
    SetDefault(batch, TOKEN_CONFIGURATION_NODE_ID, UUID());
    //TODO: SetDefault(batch, TOKEN_BLOCKCHAIN_HOME, FLAGS_path);

    leveldb::WriteOptions options;
    options.sync = true;

    leveldb::Status status;
    if(!(status = GetIndex()->Write(options, &batch)).ok()){
      LOG(FATAL) << "cannot set configuration defaults: " << status.ToString();
      return;
    }
  }

  bool Configuration::HasProperty(const std::string& name){
    if(!IsInitialized())
      return false;

    std::string slice;
    leveldb::Status status;
    if(!(status = GetIndex()->Get(leveldb::ReadOptions(), name, &slice)).ok()){
      if(status.IsNotFound()){
        DLOG(WARNING) << "cannot find property " << name;
        return false;
      }

      LOG(FATAL) << "cannot find property " << name << ": " << status.ToString();
      return false;
    }
    return true;
  }

#define DEFINE_PUT_PROPERTY(Name, Type) \
    bool Configuration::Put##Name(const std::string& name, const Type& val){ return PutProperty<Type>(name, val); }
  FOR_EACH_CONFIG_PROPERTY_TYPE(DEFINE_PUT_PROPERTY)
#undef DEFINE_PUT_PROPERTY

#define DEFINE_GET_PROPERTY(Name, Type) \
    Type Configuration::Get##Name(const std::string& name){ return GetProperty<Type>(name); }
  FOR_EACH_CONFIG_PROPERTY_TYPE(DEFINE_GET_PROPERTY)
#undef DEFINE_GET_PROPERTY
}