#include <mutex>
#include <condition_variable>

#include "configuration.h"

namespace Token{

  static std::mutex mutex_;
  static std::condition_variable cond_;
  static libconfig::Config config_;

#define LOCK_GUARD std::lock_guard<std::mutex> guard(mutex_)
#define LOCK std::unique_lock<std::mutex> lock(mutex_)
#define WAIT cond_.wait(lock)
#define SIGNAL_ONE cond_.notify_one()
#define SIGNAL_ALL cond_.notify_all()

#define ENVIRONMENT_TOKEN_LEDGER "TOKEN_LEDGER"

  bool BlockChainConfiguration::GenerateConfiguration(){
    GetProperty(PROPERTY_SERVER, libconfig::Setting::TypeGroup);
    SetServerID(UUID());

    std::set<NodeAddress> peers;
    SetPeerList(peers);

    return SaveConfiguration();
  }

  bool BlockChainConfiguration::SaveConfiguration(){
    std::string filename = GetConfigurationFilename();
    try{
      config_.writeFile(filename.data());
      return true;
    } catch(const libconfig::FileIOException& exc){
      LOG(WARNING) << "failed to save configuration to file " << filename << ": " << exc.what();
      return false;
    }
  }

  static inline bool
  ParseConfiguration(libconfig::Config& config, const std::string& filename){
    try{
      LOG(INFO) << "loading block chain configuration file from " << filename << "....";
      config.readFile(filename.c_str());
      return true;
    } catch(const libconfig::ParseException& exc){
      LOG(WARNING) << "failed to parse configuration from file " << filename << ": " << exc.what();
      return false;
    }
  }

  bool BlockChainConfiguration::LoadConfiguration(){
    std::string filename = GetConfigurationFilename();
    if(!FileExists(filename)){
      LOG(INFO) << "generating block chain configuration....";
      if(!GenerateConfiguration()){
        LOG(ERROR) << "couldn't generate configuration.";
        return false;
      }
    } else{
      if(!ParseConfiguration(config_, filename)){
        if(!GenerateConfiguration()){
          LOG(ERROR) << "couldn't generate configuration.";
          return false;
        }
      }
    }
    return true;
  }

  bool BlockChainConfiguration::Initialize(){
    if(!FileExists(TOKEN_BLOCKCHAIN_HOME)){
      if(!CreateDirectory(TOKEN_BLOCKCHAIN_HOME)){
        LOG(WARNING) << "couldn't initialize the block chain directory: " << TOKEN_BLOCKCHAIN_HOME;
        return false;
      }
    }
    return LoadConfiguration();
  }

  libconfig::Setting& BlockChainConfiguration::GetRootProperty(){
    return config_.getRoot();
  }

  libconfig::Setting& BlockChainConfiguration::GetProperty(const std::string& name, libconfig::Setting::Type type){
    libconfig::Setting& root = GetRootProperty();
    if(root.exists(name)) return root.lookup(name);
    return root.add(name, type);
  }

  bool BlockChainConfiguration::GetPeerList(std::set<NodeAddress>& results){
    LOCK_GUARD;
    if(!FLAGS_remote.empty()){
      if(!NodeAddress::ResolveAddresses(FLAGS_remote, results))
        LOG(WARNING) << "couldn't resolve peer: " << FLAGS_remote;
    }

    if(HasEnvironmentVariable(ENVIRONMENT_TOKEN_LEDGER)){
      std::string phostname = GetEnvironmentVariable(ENVIRONMENT_TOKEN_LEDGER);
      if(!phostname.empty()){
        if(!NodeAddress::ResolveAddresses(phostname, results))
          LOG(WARNING) << "couldn't resolve peer: " << phostname;
      }
    }

    libconfig::Setting& peers = GetServerProperties().lookup(PROPERTY_SERVER_PEER_LIST);
    auto iter = peers.begin();
    while(iter != peers.end()){
      results.insert(NodeAddress(std::string(iter->c_str())));
      iter++;
    }
    return true;
  }

  UUID BlockChainConfiguration::GetServerId(){
    return UUID(GetServerProperties().lookup(PROPERTY_SERVER_ID));
  }

  bool BlockChainConfiguration::SetPeerList(const std::set<NodeAddress>& peers){
    LOCK_GUARD;
    libconfig::Setting& server = GetServerProperties();
    if(server.exists(PROPERTY_SERVER_PEER_LIST)){
      server.remove(PROPERTY_SERVER_PEER_LIST);
    }
    libconfig::Setting& property = server.add(PROPERTY_SERVER_PEER_LIST, libconfig::Setting::TypeList);
    for(auto& it : peers){
      property.add(libconfig::Setting::TypeString) = it.ToString();
    }

    if(!SaveConfiguration()){
      LOG(WARNING) << "couldn't save the configuration";
      return false;
    }
    return true;
  }

  bool BlockChainConfiguration::SetServerID(const UUID& uuid){
    LOCK_GUARD;
    libconfig::Setting& server = GetServerProperties();
    if(server.exists(PROPERTY_SERVER_ID)){
      server.remove(PROPERTY_SERVER_ID);
    }
    server.add(PROPERTY_SERVER_ID, libconfig::Setting::Type::TypeString) = uuid.ToString();
    if(!SaveConfiguration()){
      LOG(WARNING) << "couldn't save the configuration";
      return false;
    }
    return true;
  }
}