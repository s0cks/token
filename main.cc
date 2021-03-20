#include "pool.h"
#include "miner.h"
#include "wallet.h"
#include "keychain.h"
#include "blockchain.h"
#include "configuration.h"
#include "job/scheduler.h"

#include "filesystem.h"

#include "crash/crash_report.h"

#include "rpc/rpc_server.h"
#include "peer/peer_session_manager.h"
#include "http/http_service_health.h"
#include "http/http_service_rest.h"

#ifdef TOKEN_ENABLE_ELASTICSEARCH
  #include "elastic/elastic_client.h"
#endif//TOKEN_ENABLE_ELASTICSEARCH

DEFINE_string(path, "", "The path for the local ledger to be stored in.");
DEFINE_bool(enable_snapshots, false, "Enable snapshots of the block chain");
DEFINE_int32(num_workers, 4, "Define the number of worker pool threads");
DEFINE_int64(mining_interval, 1000 * 60 * 1, "The amount of time between mining blocks in milliseconds.");
DEFINE_string(remote, "", "The hostname for the remote ledger to join.");
DEFINE_int32(server_port, 0, "The port to use for the RPC serer.");
DEFINE_int32(num_peers, 2, "The max number of peers to connect to.");
DEFINE_int32(healthcheck_port, 0, "The port to use for the health check service.");
DEFINE_int32(service_port, 0, "The port for the ledger controller service.");

#ifdef TOKEN_DEBUG
  DEFINE_bool(fresh, false, "Initialize the BlockChain w/ a fresh chain [Debug]");
  DEFINE_bool(append_test, false, "Append a test block upon startup [Debug]");
  DEFINE_bool(verbose, false, "Turn on verbose logging [Debug]");
  DEFINE_bool(no_mining, false, "Turn off block mining [Debug]");
#endif//TOKEN_DEBUG

static inline void
InitializeLogging(char *arg0){
  using namespace token;
  google::LogToStderr();
  google::InitGoogleLogging(arg0);
}

#ifdef TOKEN_DEBUG
  static inline bool
  AppendDummy(const std::string& user, int total_spends){
    using namespace token;

    WalletManager* wallets = WalletManager::GetInstance();

    sleep(10);

    LOG(INFO) << "spending " << total_spends << " unclaimed transactions";

    Wallet wallet;
    if(!wallets->GetWallet(user, wallet)){
      LOG(WARNING) << "couldn't get the wallet for VenueA";
      return false;
    }

    ObjectPoolPtr pool = ObjectPool::GetInstance();

    int64_t idx = 0;
    for(auto& it : wallet){
      UnclaimedTransactionPtr utxo = pool->GetUnclaimedTransaction(it);

      LOG(INFO) << "spending: " << utxo->GetReference();

      InputList inputs = {};
      OutputList outputs = {
        Output("TestUser2", "TestToken2")
      };
      TransactionPtr tx = Transaction::NewInstance(inputs, outputs);

      Hash hash = tx->GetHash();
      if(!pool->PutTransaction(hash, tx)){
        LOG(WARNING) << "cannot add new transaction " << hash << " to object pool.";
        return false;
      }

      if(++idx == total_spends)
        return true;
    }
    return false;
  }
#endif//TOKEN_DEBUG

static inline token::User
RandomUser(const std::vector<token::User>& users){
  static std::default_random_engine engine;
  static std::uniform_int_distribution<int> distribution(0, users.size());
  return users[distribution(engine)];
}

//TODO:
// - create global environment teardown and deconstruct routines
// - validity/consistency checks on block chain data
// - safer/better shutdown/terminate routines
int
main(int argc, char **argv){
  using namespace token;
  // Install Signal Handlers
  SignalHandlers::Initialize();

  // Parse Command Line Arguments
  gflags::ParseCommandLineFlags(&argc, &argv, true);

  // Initialize the Logging Framework
  InitializeLogging(argv[0]);

  // Create the home directory if it doesn't exist
  if(!FileExists(TOKEN_BLOCKCHAIN_HOME)){
    if(!CreateDirectory(TOKEN_BLOCKCHAIN_HOME)){
      std::stringstream ss;
      ss << "Cannot create block chain home directory: " << TOKEN_BLOCKCHAIN_HOME;
      CrashReport::PrintNewCrashReport(ss);
      return EXIT_FAILURE;
    }
  }

  // Load the configuration
  if(!ConfigurationManager::Initialize(TOKEN_BLOCKCHAIN_HOME)){
    CrashReport::PrintNewCrashReport("Failed to initialize the configuration manager.");
    return EXIT_FAILURE;
  }

  // ~16.07s on boot for 30k Tokens (not initialized)
  // ~2s on boot for 30k tokens (initialized)
  #ifdef TOKEN_DEBUG
    //TODO: BannerPrinter::PrintBanner();
  #endif//TOKEN_DEBUG

  // Start the Health Check Service
  if(IsValidPort(FLAGS_healthcheck_port) && !HttpHealthService::Start()){
    CrashReport::PrintNewCrashReport("Failed to start the health check service.");
    return EXIT_FAILURE;
  }

  // Initialize the job scheduler
  if(!JobScheduler::Initialize()){
    CrashReport::PrintNewCrashReport("Failed to initialize the job scheduler.");
    return EXIT_FAILURE;
  }

  // Initialize the keychain
  if(!Keychain::Initialize()){
    CrashReport::PrintNewCrashReport("Failed to initialize the blockchain keychain.");
    return EXIT_FAILURE;
  }

  // Initialize the object pool
  if(!ObjectPool::Initialize()){
    CrashReport::PrintNewCrashReport("Failed to load the object pool.");
    return EXIT_FAILURE;
  }

  // Initialize the wallet manager
  if(!WalletManager::Initialize()){
    CrashReport::PrintNewCrashReport("Failed to initialize the wallet manager.");
    return EXIT_FAILURE;
  }

  // Initialize the block chain
  if(!BlockChain::Initialize()){
    CrashReport::PrintNewCrashReport("Failed to initialize the block chain.");
    return EXIT_FAILURE;
  }

  // Start the rpc server & connect to peers.
  if(IsValidPort(FLAGS_server_port) && !LedgerServer::Start()){
    CrashReport::PrintNewCrashReport("Failed to start the rpc.");
    return EXIT_FAILURE;
  }

  if(!PeerSessionManager::Initialize()){
    CrashReport::PrintNewCrashReport("Failed to initialize the peer session manager.");
    return EXIT_FAILURE;
  }

  // Start the controller service if enabled
  if(IsValidPort(FLAGS_service_port) && !HttpRestService::Start()){
    CrashReport::PrintNewCrashReport("Failed to start the controller service.");
    return EXIT_FAILURE;
  }

#ifdef TOKEN_DEBUG
  sleep(3);
  LOG(INFO) << "current time: " << FormatTimestampReadable(Clock::now());
  LOG(INFO) << "home: " << TOKEN_BLOCKCHAIN_HOME;
  LOG(INFO) << "node: " << ConfigurationManager::GetNodeID();

  PeerList peers;
  ConfigurationManager::GetInstance()->GetPeerList(TOKEN_CONFIGURATION_NODE_PEERS, peers);
  LOG(INFO) << "peers: " << peers;
  LOG(INFO) << "number of blocks in the chain: " << BlockChain::GetInstance()->GetNumberOfBlocks();

  ObjectPoolPtr pool = ObjectPool::GetInstance();
  if(TOKEN_VERBOSE){
    pool->PrintBlocks();
    pool->PrintTransactions();
    pool->PrintUnclaimedTransactions();
  } else{
    LOG(INFO) << "number of blocks in the pool: " << pool->GetNumberOfBlocks();
    LOG(INFO) << "number of transactions in the pool: " << pool->GetNumberOfTransactions();
    LOG(INFO) << "number of unclaimed transactions in the pool: " << pool->GetNumberOfUnclaimedTransactions();
  }

  if(FLAGS_append_test && !AppendDummy("VenueA", 2)){
    CrashReport::PrintNewCrashReport("Cannot append dummy transactions.");
    return EXIT_FAILURE;
  }
#endif//TOKEN_DEBUG

//  if(!PeerSessionManager::Shutdown()){
//    CrashReport::PrintNewCrashReport("Cannot shutdown peers.");
//    return EXIT_FAILURE;
//  }

  if(!FLAGS_no_mining){
    if(!BlockMiner::Initialize())
      CrashReport::PrintNewCrashReport("Cannot initialize the block miner.");

    if(!BlockMiner::GetInstance()->Run()){
      CrashReport::PrintNewCrashReport("Cannot run the block miner.");
    }
  }

  if(!PeerSessionManager::WaitForShutdown()){
    CrashReport::PrintNewCrashReport("Cannot shutdown the peer session manager.");
    return EXIT_FAILURE;
  }

  if(IsValidPort(FLAGS_server_port) && LedgerServer::IsServerRunning() && !LedgerServer::WaitForShutdown()){
    CrashReport::PrintNewCrashReport("Cannot join the rpc thread.");
    return EXIT_FAILURE;
  }

  if(IsValidPort(FLAGS_service_port) && HttpRestService::IsServiceRunning() && !HttpRestService::WaitForShutdown()){
    CrashReport::PrintNewCrashReport("Cannot join the controller service thread.");
    return EXIT_FAILURE;
  }

  if(IsValidPort(FLAGS_healthcheck_port) && HttpHealthService::IsServiceRunning() && !HttpHealthService::WaitForShutdown()){
    CrashReport::PrintNewCrashReport("Cannot join the health check service thread.");
    return EXIT_FAILURE;
  }
  return EXIT_SUCCESS;
}