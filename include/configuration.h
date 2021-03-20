#ifndef TOKEN_CONFIGURATION_H
#define TOKEN_CONFIGURATION_H

#include <set>
#include <vector>
#include <leveldb/db.h>
#include <leveldb/status.h>
#include <leveldb/write_batch.h>

#include "common.h"
#include "address.h"
#include "peer/peer.h"

#include "atomic/relaxed_atomic.h"

namespace token{
#define TOKEN_CONFIGURATION_NODE_ID "Node.Id"
#define TOKEN_CONFIGURATION_NODE_PEERS "Node.Peers"
#define TOKEN_CONFIGURATION_BLOCKCHAIN_HOME "BlockChain.Home"

#define FOR_EACH_CONFIGURATION_MANAGER_STATE(V) \
  V(Uninitialized)                              \
  V(Initializing)                               \
  V(Initialized)

  static inline std::string
  GetConfigurationManagerFilename(){
    //TODO: cross-platform support
    std::stringstream ss;
    ss << TOKEN_BLOCKCHAIN_HOME;
    ss << "/config";
    return ss.str();
  }

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
    RelaxedAtomic<State> state_;
    leveldb::DB* index_;

    /**
     * Returns the index for the ConfigurationManager
     * @see leveldb::DB
     * @return The index for the ConfigurationManager
     */
     inline leveldb::DB*
     GetIndex() const{
       return index_;
     }

    /**
     * Sets the ConfigurationManager's state
     *
     * @see State
     * @param state - The desired state of the ConfigurationManager
     */
    void SetState(const State& state){
      state_ = state;
    }

    /**
     * Sets the default values for all known configuration properties.
     *
     * @see leveldb::Status
     * @return The leveldb::Status for the batch write
     */
    leveldb::Status SetDefaults(const std::string& home_dir);//TODO: refactor

    /**
     * Initializes the ConfigurationManager in a specific directory
     *
     * @see leveldb::Status
     * @param filename - The filename to initialize the ConfigurationManager in
     * @return The leveldb::Status for the initialization
     */
     leveldb::Status InitializeIndex(const std::string& filename);
   public:
    ConfigurationManager():
      state_(State::kUninitializedState),
      index_(nullptr){}
    virtual ~ConfigurationManager() = default;

    /**
     * Returns the state of the ConfigurationManager
     *
     * @see State
     * @return The current state of the ConfigurationManager
     */
    State GetState() const{
      return (State)state_;
    }

    /**
     * Returns whether or not the ConfigurationManager has a property
     *
     * @param name - The name of the property to check
     * @return True if the property was found, false otherwise.
     */
    bool HasProperty(const std::string& name) const;

    /**
     * Sets a property to a std::string value.
     *
     * @param name - The name of the property to set
     * @param val - The new value for the property
     * @return True if the property was set, false otherwise
     */
     bool PutProperty(const std::string& name, const std::string& val) const;

    /**
     * Set a property to a UUID value.
     *
     * @param name - The name of the property to set
     * @param val - The new value for the property
     * @return True if the property was set, false otherwise.
     */
     bool PutProperty(const std::string& name, const UUID& val) const;

    /**
     * Sets a property to a NodeAddress value.
     *
     * @param name - The name of the property to set
     * @param val - The new value for the property
     * @return True if the property was set, false otherwise.
     */
    bool PutProperty(const std::string& name, const NodeAddress& val) const;

    /**
     * Set a property to a PeerList value.
     *
     * @param name - The name of the property to set
     * @param val - The new value for the property
     * @return True if the property was set, false otherwise.
     */
    bool PutProperty(const std::string& name, const PeerList& val) const;

    /**
     * Gets a property as a PeerList.
     *
     * @param name - The name of the property to get
     * @param peers - Reference to a PeerList to put the results in
     * @return True if the property was able to be resolve, false otherwise.
     */
    bool GetPeerList(const std::string& name, PeerList& peers) const;

    /**
     * Gets a property as a UUID.
     *
     * @param name - The name of the property to get
     * @return True if the property was able to be resolved, false otherwise.
     */
    bool GetUUID(const std::string& name, UUID& value) const;

    /**
     * Gets a property as a std::string
     *
     * @param name - The name of the property to get
     * @return True if the property was able to be resolved, false otherwise.
     */
    bool GetString(const std::string& name, std::string& value) const;

    // define ConfigurationManager state checks using macros
#define DEFINE_CHECK(Name) \
    inline bool Is##Name##State() const{ return GetState() == State::k##Name##State; }
    FOR_EACH_CONFIGURATION_MANAGER_STATE(DEFINE_CHECK)
#undef DEFINE_CHECK

    static bool Initialize(const std::string& filename=GetConfigurationManagerFilename());//TODO: document
    static ConfigurationManager* GetInstance();//TODO: document

    static inline UUID
    GetNodeID(){
      UUID node_id;
      GetInstance()->GetUUID(TOKEN_CONFIGURATION_NODE_ID, node_id);
      return node_id;
    }
  };
}

#endif //TOKEN_CONFIGURATION_H