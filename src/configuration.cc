#include <leveldb/db.h>
#include <leveldb/status.h>

#include "buffer.h"
#include "configuration.h"

namespace token{
  namespace config{
    static leveldb::DB* index_ = nullptr;
    static RelaxedAtomic<State> state_{ State::kUninitializedState };

    static inline State
    GetState(){
      return (State)state_;
    }

    static inline void
    SetState(const State& val){
      DLOG(INFO) << "setting the configuration state to " << val << "....";
      state_ = val;
    }

    static inline leveldb::DB*
    GetIndex(){
      return index_;
    }

    void Initialize(const std::string& filename){
      if(!IsUninitializedState()){
        LOG(FATAL) << "cannot re-initialize the configuration.";
        return;
      }

      bool first_initialization = !FileExists(filename);
      SetState(State::kInitializingState);

      leveldb::Options options;
      options.create_if_missing = true;

      leveldb::Status status;
      if(!(status = leveldb::DB::Open(options, filename, &index_)).ok()){
        LOG(FATAL) << "couldn't load the configuration index from " << filename << ": " << status.ToString();
        return;
      }

      if(first_initialization){
        SetDefaults();
        DLOG(INFO) << "initialized the configuration in: " << filename;
      } else{
        DLOG(INFO) << "loaded the configuration from: " << filename;
      }
      SetState(State::kInitializedState);
    }

#define DEFINE_SET_DEFAULT(Name, Type) \
    static inline void SetDefault(leveldb::WriteBatch& batch, const std::string& name, const Type& val){ return batch.Put(name, (leveldb::Slice)val); }
    FOR_EACH_CONFIG_PROPERTY_TYPE(DEFINE_SET_DEFAULT)
#undef DEFINE_SET_DEFAULT

    static inline void
    SetDefaultProperty(leveldb::WriteBatch& batch, const std::string& name, const PeerList& val){
      internal::BufferPtr buffer = internal::NewBuffer(GetBufferSize(val));
      if(!buffer->PutPeerList(val)){
        LOG(WARNING) << "cannot serialize peer list to buffer of size " << buffer->ToString();
        return;
      }
      batch.Put(name, buffer->AsSlice());
    }

    void SetDefaults(){
      leveldb::WriteBatch batch;
      SetDefault(batch, TOKEN_CONFIGURATION_NODE_ID, UUID());
      SetDefault(batch, TOKEN_BLOCKCHAIN_HOME, FLAGS_path);

      PeerList peers;
      if(!FLAGS_remote.empty()){
        DLOG_CONFIG_IF(WARNING, !NodeAddress::ResolveAddresses(FLAGS_remote, peers)) << "couldn't resolve remote address: " << FLAGS_remote;
      }
      SetDefaultProperty(batch, TOKEN_CONFIGURATION_NODE_PEERS, peers);

      leveldb::WriteOptions options;
      options.sync = true;

      leveldb::Status status;
      if(!(status = GetIndex()->Write(options, &batch)).ok()){
        LOG(FATAL) << "cannot set configuration defaults: " << status.ToString();
        return;
      }
    }

    template<class T>
    static inline T
    GetProperty(const std::string& name){
      if(!IsInitializedState())
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
    static inline bool
    PutProperty(const std::string& name, const T& val){
      if(!IsInitializedState())
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

    static inline bool
    PutProperty(const std::string& name, const internal::BufferPtr& val){
      if(!IsInitializedState())
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

    bool HasProperty(const std::string& name){
      if(!IsInitializedState())
        return false;
      std::string slice;
      leveldb::Status status;
      if(!(status = GetIndex()->Get(leveldb::ReadOptions(), name, &slice)).ok()){
        if(status.IsNotFound()){
          DLOG(WARNING) << "couldn't find configuration property '" << name << "'";
          return false;
        }

        LOG(FATAL) << "error checking for property '" << name << "': " << status.ToString();
        return false;
      }
      return true;
    }

#define DEFINE_PUT_PROPERTY(Name, Type) \
    bool PutProperty(const std::string& name, const Type& val){ return PutProperty<Type>(name, val); }
    FOR_EACH_CONFIG_PROPERTY_TYPE(DEFINE_PUT_PROPERTY)
#undef DEFINE_PUT_PROPERTY

    bool PutProperty(const std::string& name, const PeerList& val){
      auto slice = internal::NewBuffer(GetBufferSize(val));
      if(!slice->PutPeerList(val)){
        DLOG(WARNING) << "cannot serialize peer list to buffer.";
        return false;
      }
      return PutProperty(name, slice);
    }

#define DEFINE_GET_PROPERTY(Name, Type) \
    Type Get##Name(const std::string& name){ return GetProperty<Type>(name); }
    FOR_EACH_CONFIG_PROPERTY_TYPE(DEFINE_GET_PROPERTY)
#undef DEFINE_GET_PROPERTY

    bool GetPeerList(const std::string& name, PeerList& results){
      if(!IsInitializedState())
        return false;
      std::string slice;
      leveldb::Status status;
      if(!(status = GetIndex()->Get(leveldb::ReadOptions(), name, &slice)).ok()){
        if(status.IsNotFound()){
          DLOG(WARNING) << "couldn't find property '" << name << "'";
          return false;
        }

        LOG(FATAL) << "couldn't get property '" << name << "': " << status.ToString();
        return false;
      }

      BufferPtr data = internal::CopyBufferFrom(slice);
      return data->GetPeerList(results);
    }

#define DEFINE_STATE_CHECK(Name) \
    bool Is##Name##State(){ return GetState() == State::k##Name##State; }
    FOR_EACH_CONFIGURATION_STATE(DEFINE_STATE_CHECK)
#undef DEFINE_STATE_CHECK
  }
}