#ifndef TOKEN_CONFIGURATION_H
#define TOKEN_CONFIGURATION_H

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

#define FOR_EACH_CONFIGURATION_STATE(V) \
  V(Uninitialized)                              \
  V(Initializing)                               \
  V(Initialized)

#define FOR_EACH_CONFIG_PROPERTY_TYPE(V) \
  V(Hash, Hash)                          \
  V(UUID, UUID)                          \
  V(String, std::string)

  namespace config{
    enum State{
#define DEFINE_STATE(Name) k##Name##State,
      FOR_EACH_CONFIGURATION_STATE(DEFINE_STATE)
#undef DEFINE_STATE
    };

    inline std::ostream&
    operator<<(std::ostream& stream, const State& state){
      switch(state){
#define DEFINE_TOSTRING(Name) \
        case State::k##Name##State: \
          return stream << #Name;
        FOR_EACH_CONFIGURATION_STATE(DEFINE_TOSTRING)
#undef DEFINE_TOSTRING
        default:
          return stream << "Unknown";
      }
    }

    static inline std::string
    GetPath(){
      //TODO: fixme
      std::stringstream filename;
      filename << "" << "/config";
      return filename.str();
    }

    void SetDefaults();
    void Initialize(const std::string& filename=GetPath());
    bool HasProperty(const std::string& name);

#define DECLARE_PUT_PROPERTY(Name, Type) \
    bool PutProperty(const std::string& name, const Type& val);
    FOR_EACH_CONFIG_PROPERTY_TYPE(DECLARE_PUT_PROPERTY)
#undef DECLARE_PUT_PROPERTY

#define DECLARE_GET_PROPERTY(Name, Type) \
    Type Get##Name(const std::string& name);
    FOR_EACH_CONFIG_PROPERTY_TYPE(DECLARE_GET_PROPERTY)
#undef DECLARE_GET_PROPERTY

#define DECLARE_STATE_CHECK(Name) \
    bool Is##Name##State();
    FOR_EACH_CONFIGURATION_STATE(DECLARE_STATE_CHECK)
#undef DECLARE_STATE_CHECK

    static inline UUID
    GetServerNodeID(){
      return GetUUID(TOKEN_CONFIGURATION_NODE_ID);
    }
  }
}

#endif //TOKEN_CONFIGURATION_H