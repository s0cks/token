#ifndef TOKEN_CONFIGURATION_H
#define TOKEN_CONFIGURATION_H

#include <set>
#include <vector>
#include <leveldb/status.h>
#include <leveldb/write_batch.h>

#include "address.h"
#include "peer/peer.h"

namespace token{
#define TOKEN_CONFIGURATION_NODE_ID "Node.Id"
#define TOKEN_CONFIGURATION_NODE_PEERS "Node.Peers"

#define FOR_EACH_CONFIGURATION_MANAGER_STATE(V) \
  V(Uninitialized)                              \
  V(Initializing)                               \
  V(Initialized)

  class ConfigurationManager{
   public:
    enum State{
#define DEFINE_STATE(Name) k##Name##State,
      FOR_EACH_CONFIGURATION_MANAGER_STATE(DEFINE_STATE)
#undef DEFINE_STATE
    };

    friend std::ostream& operator<<(std::ostream& stream, const State& state){
      switch(state){
#define DEFINE_TOSTRING(Name) \
        case State::k##Name##State: \
          return stream << #Name;
        FOR_EACH_CONFIGURATION_MANAGER_STATE(DEFINE_TOSTRING)
#undef DEFINE_TOSTRING
        default:
          return stream << "Unknown";
      }
    }
   private:
    ConfigurationManager() = delete;

    /**
     * Sets the ConfigurationManager's state
     *
     * @see State
     * @param state - The desired state of the ConfigurationManager
     */
    static void SetState(const State& state);

    /**
     * Sets the default values for all known configuration properties.
     */
    static leveldb::Status SetDefaults();
   public:
    ~ConfigurationManager() = delete;

    /**
     * Returns the state of the ConfigurationManager
     *
     * @see State
     * @return The current state of the ConfigurationManager
     */
    static State GetState();

    /**
     * Initializes the ConfigurationManager
     *
     * @return true if successful otherwise, false
     */
    static bool Initialize();
    static bool HasProperty(const std::string& name);
    static bool SetProperty(const std::string& name, const UUID& val);
    static bool SetProperty(const std::string& name, const PeerList& val);
    static bool GetPeerList(const std::string& name, PeerList& peers);
    static UUID GetID(const std::string& name);

#define DEFINE_CHECK(Name) \
    static inline bool Is##Name##State(){ return GetState() == State::k##Name##State; }
    FOR_EACH_CONFIGURATION_MANAGER_STATE(DEFINE_CHECK)
#undef DEFINE_CHECK
  };
}

#endif //TOKEN_CONFIGURATION_H