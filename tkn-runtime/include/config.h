#ifndef TOKEN_CONFIG_H
#define TOKEN_CONFIG_H

#include <set>
#include <vector>
#include <leveldb/db.h>
#include <leveldb/status.h>
#include <leveldb/write_batch.h>

#include "uuid.h"
#include "hash.h"
#include "address.h"
#include "atomic/relaxed_atomic.h"

namespace token{
#define LOG_CONFIG(LevelName) \
  LOG(LevelName) << "[config] "

#define DLOG_CONFIG(LevelName) \
  DLOG(LevelName) << "[config] "

#define DLOG_CONFIG_IF(LevelName, Condition) \
  DLOG_IF(LevelName, Condition)

#define TOKEN_CONFIGURATION_NODE_ID "Node.Id"
#define TOKEN_CONFIGURATION_NODE_PEERS "Node.Peers"
#define TOKEN_CONFIGURATION_BLOCKCHAIN_HOME "BlockChain.Home"

#define FOR_EACH_CONFIG_STATE(V) \
  V(Uninitialized)                              \
  V(Initializing)                               \
  V(Initialized)

#define FOR_EACH_CONFIG_PROPERTY_TYPE(V) \
  V(Hash, Hash)                          \
  V(UUID, UUID)                          \
  V(String, std::string)

  class Configuration{
  public:
    enum State{
#define DEFINE_STATE(Name) k##Name,
      FOR_EACH_CONFIG_STATE(DEFINE_STATE)
#undef DEFINE_STATE
    };

    friend std::ostream&
    operator<<(std::ostream& stream, const State& state){
      switch(state){
#define DEFINE_TOSTRING(Name) \
        case State::k##Name: \
          return stream << #Name;
        FOR_EACH_CONFIG_STATE(DEFINE_TOSTRING)
#undef DEFINE_TOSTRING
        default:
          return stream << "Unknown";
      }
    }
  protected:
    atomic::RelaxedAtomic<State> state_;
    leveldb::DB* index_;
    std::string filename_;

    inline leveldb::DB*
    GetIndex() const{
      return index_;
    }

    void SetState(const State& state){
      state_ = state;
    }

    void SetDefaults();
    void Initialize();

    template<class T>
    inline T
    GetProperty(const std::string& name){
      if(!IsInitialized())
        return T();

      std::string slice;
      leveldb::Status status;
      if(!(status = GetIndex()->Get(leveldb::ReadOptions(), name, &slice)).ok()){
        if(status.IsNotFound()){
          LOG(WARNING) << "couldn't find the configuration property '" << name << "'";
          return T();
        }

        LOG(FATAL) << "error getting the configuration property '" << name << "': " << status.ToString();
        return T();
      }
      return T(slice);
    }

    template<class T>
    inline bool
    PutProperty(const std::string& name, const T& val){
      if(!IsInitialized())
        return false;

      leveldb::Slice value = (leveldb::Slice)val;

      leveldb::WriteOptions options;
      options.sync = true;

      leveldb::Status status;
      if(!(status = GetIndex()->Put(options, name, value)).ok()){
        LOG(FATAL) << "error setting the configuration property '" << name << "': " << status.ToString();
        return false;
      }

      DLOG(INFO) << "set the " << name << " configuration property to: " << val;
      return true;
    }

    template<class T>
    inline bool
    PutProperty(const std::string& name, const std::shared_ptr<T>& val){
      if(!IsInitialized())
        return false;

      leveldb::WriteOptions options;
      options.sync = true;

      leveldb::Status status;
      if(!(status = GetIndex()->Put(options, name, val->AsSlice())).ok()){
        LOG(FATAL) << "error setting the configuration property '" << name << "': " << status.ToString();
        return false;
      }

      DLOG(INFO) << "set the " << name << " configuration property to: " << val;
      return true;
    }
  public:
    explicit Configuration(const std::string& filename);
    virtual ~Configuration(){
      delete index_;
    }

    State GetState() const{
      return (State)state_;
    }

    std::string GetFilename() const{
      return filename_;
    }

    virtual bool HasProperty(const std::string& name);
#define DECLARE_CONFIG_PUT_PROPERTY(Name, Type) \
    virtual bool Put##Name(const std::string& name, const Type& val);
#define DECLARE_CONFIG_GET_PROPERTY(Name, Type) \
    virtual Type Get##Name(const std::string& name);
#define DECLARE_CONFIG_TYPE_METHODS(Name, Type) \
    DECLARE_CONFIG_PUT_PROPERTY(Name, Type) \
    DECLARE_CONFIG_GET_PROPERTY(Name, Type)
    FOR_EACH_CONFIG_PROPERTY_TYPE(DECLARE_CONFIG_TYPE_METHODS)
#undef DECLARE_CONFIG_TYPE_METHODS
#undef DECLARE_CONFIG_GET_PROPERTY
#undef DECLARE_CONFIG_PUT_PROPERTY

#define DEFINE_STATE_CHECK(Name) \
    inline bool Is##Name() const{ return GetState() == State::k##Name; }
    FOR_EACH_CONFIG_STATE(DEFINE_STATE_CHECK)
#undef DEFINE_STATE_CHECK
  };
}

#endif //TOKEN_CONFIG_H