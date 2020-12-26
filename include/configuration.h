#ifndef TOKEN_CONFIGURATION_H
#define TOKEN_CONFIGURATION_H

#include <set>
#include <vector>
#include <libconfig.h++>
#include "uuid.h"
#include "address.h"

namespace Token{
#define BLOCKCHAIN_CONFIGURATION_FILENAME "token.cfg"

// Health Check Properties
#define PROPERTY_HEALTHCHECK "HealthCheck"
/*
 * HealthCheck.Port := ${port}+1
 */
#define PROPERTY_HEALTHCHECK_PORT "Port"
#define PROPERTY_HEALTHCHECK_PORT_DEFAULT (FLAGS_port + 1)

// Server Properties
#define PROPERTY_SERVER "Server"
/*
 * Server.Id = UUID();
 */
#define PROPERTY_SERVER_ID "Id"
#define PROPERTY_SERVER_ID_DEFAULT (UUID())

/*
 * Server.CallbackAddress = "localhost:${port}";
 */
#define PROPERTY_SERVER_CALLBACK_ADDRESS "CallbackAddress"
#define PROPERTY_SERVER_CALLBACK_ADDRESS_DEFAULT ()

#define PROPERTY_SERVER_PEER_LIST "Peers"
#define PROPERTY_SERVER_PEER_LIST_DEFAULT ({})

#define PROPERTY_SERVER_MAXPEERS "MaxNumberOfPeers"
#define PROPERTY_SERVER_MAXPEERS_DEFAULT 4

    class BlockChainConfiguration{
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

        static inline libconfig::Setting&
        GetHealthCheckProperties(){
            return GetProperty(PROPERTY_HEALTHCHECK, libconfig::Setting::TypeGroup);
        }
    public:
        ~BlockChainConfiguration() = delete;
        static bool Initialize();

        // Health Check Service
        // HealthCheck.Port
        static int32_t GetHealthCheckPort();
        static bool SetHealthCheckPort(int32_t port);

        // Server
        // Server.Id
        static UUID GetSererID();
        static bool SetServerID(const UUID& uuid);
        // Server.CallbackAddress
        static NodeAddress GetServerCallbackAddress();
        static bool SetServerCallbackAddress(const NodeAddress& address);
        // Server.MaxNumberOfPeers
        static int32_t GetMaxNumberOfPeers();
        static bool SetMaxNumberOfPeers(int32_t value);
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