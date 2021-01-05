#ifndef TOKEN_CONFIGURATION_H
#define TOKEN_CONFIGURATION_H

#include <set>
#include <vector>
#include <libconfig.h++>
#include "uuid.h"
#include "address.h"

namespace Token{
#define BLOCKCHAIN_CONFIGURATION_FILENAME "token.cfg"

// Server Properties
#define PROPERTY_SERVER "Server"
#define PROPERTY_SERVER_ID "Id"
#define PROPERTY_SERVER_CALLBACK_ADDRESS "CallbackAddress"
#define PROPERTY_SERVER_PEER_LIST "Peers"
#define PROPERTY_SERVER_MAXPEERS "MaxNumberOfPeers"
#define PROPERTY_SERVER_MAXPEERS_DEFAULT 4

  class BlockChainConfiguration{
    //TODO: refactor
   private:
    BlockChainConfiguration() = delete;
    static bool SaveConfiguration();
    static bool LoadConfiguration();
    static bool GenerateConfiguration();

    static libconfig::Setting& GetRootProperty();
    static libconfig::Setting& GetProperty(const std::string& name, libconfig::Setting::Type type);

    static inline libconfig::Setting&
    GetServerProperties(){
      return GetProperty(PROPERTY_SERVER, libconfig::Setting::TypeGroup);
    }
   public:
    ~BlockChainConfiguration() = delete;
    static bool Initialize();

    // Server
    // Server.Id
    static UUID GetSererID();
    static bool SetServerID(const UUID& uuid);

    // Server.Peers
    static bool GetPeerList(std::set<NodeAddress>& peers);
    static bool SetPeerList(const std::set<NodeAddress>& peers);

    static inline std::string
    GetConfigurationFilename(){
      return FLAGS_path + "/" + BLOCKCHAIN_CONFIGURATION_FILENAME;
    }
  };
}

#endif //TOKEN_CONFIGURATION_H