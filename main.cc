#include <thread>
#include "pool.h"
#include "wallet.h"
#include "keychain.h"
#include "discovery.h"
#include "blockchain.h"
#include "configuration.h"
#include "job/scheduler.h"
#include "utils/crash_report.h"

#ifdef TOKEN_ENABLE_SERVER
  #include "server/server.h"
  #include "peer/peer_session_manager.h"
#endif//TOKEN_ENABLE_SERVER

#ifdef TOKEN_ENABLE_HEALTH_SERVICE
  #include "http/healthcheck_service.h"
#endif//TOKEN_ENABLE_HEALTHCHECK

#ifdef TOKEN_ENABLE_REST_SERVICE
  #include "http/rest_service.h"
#endif//TOKEN_ENABLE_REST

static inline void
InitializeLogging(char *arg0){
  using namespace Token;
  google::LogToStderr();
  google::InitGoogleLogging(arg0);
}

static inline bool
AppendDummy(int total_spends){
  using namespace Token;
  sleep(5);

  Wallet wallet;
  if(!WalletManager::GetWallet("VenueA", wallet)){
    LOG(WARNING) << "couldn't get the wallet for VenueA";
    return false;
  }

  LOG(INFO) << "spending " << total_spends << " unclaimed transactions";

  int64_t idx = 0;
  for(auto& it : wallet){
    UnclaimedTransactionPtr utxo = ObjectPool::GetUnclaimedTransaction(it);

    LOG(INFO) << "spending " << it << " (" << utxo->GetReference() << ")";
    InputList inputs = {
      Input(utxo->GetReference(), utxo->GetUser()),
    };
    OutputList outputs = {
      Output("TestUser2", "TestToken2")
    };
    TransactionPtr tx = Transaction::NewInstance(idx++, inputs, outputs);
    Hash hash = tx->GetHash();
    if(!ObjectPool::PutTransaction(hash, tx)){
      LOG(WARNING) << "cannot add new transaction " << hash << " to object pool.";
      return false;
    }

    if(idx == total_spends)
      return true;
  }
  return false;
}

//TODO:
// - create global environment teardown and deconstruct routines
// - validity/consistency checks on block chain data
// - safer/better shutdown/terminate routines
int
main(int argc, char **argv){
  using namespace Token;
  // Install Signal Handlers
  SignalHandlers::Initialize();

  // Parse Command Line Arguments
  gflags::ParseCommandLineFlags(&argc, &argv, true);

  // Initialize the Logging Framework
  InitializeLogging(argv[0]);

  // ~16.07s on boot for 30k Tokens (not initialized)
  // ~2s on boot for 30k tokens (initialized)
  #ifdef TOKEN_DEBUG
    BannerPrinter::PrintBanner();
  #endif//TOKEN_DEBUG

  // Load the configuration
  if(!BlockChainConfiguration::Initialize()){
    CrashReport::PrintNewCrashReport("Failed to load the configuration.");
    return EXIT_FAILURE;
  }

  // Start the health service if enabled
  #ifdef TOKEN_ENABLE_HEALTH_SERVICE
    if(IsValidPort(FLAGS_healthcheck_port) && !HealthCheckService::Start()){
      CrashReport::PrintNewCrashReport("Failed to start the health check service.");
      return EXIT_FAILURE;
    }
  #endif//TOKEN_ENABLE_HEALTH_SERVICE

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

  // Start the block discovery thread
  if(!BlockDiscoveryThread::Start()){
    CrashReport::PrintNewCrashReport("Failed to start the block discovery thread.");
    return EXIT_FAILURE;
  }

  // Start the server if enabled
  #ifdef TOKEN_ENABLE_SERVER
    if(IsValidPort(FLAGS_server_port) && !Server::Start()){
      CrashReport::PrintNewCrashReport("Failed to start the server.");
      return EXIT_FAILURE;
    }

    if(IsValidPort(FLAGS_server_port) && !PeerSessionManager::Initialize()){
      CrashReport::PrintNewCrashReport("Failed to initialize the peer session manager.");
      return EXIT_FAILURE;
    }
  #endif//TOKEN_ENABLE_SERVER

  // Start the rest service if enabled
  #ifdef TOKEN_ENABLE_REST_SERVICE
    if(IsValidPort(FLAGS_service_port) && !RestService::Start()){
      CrashReport::PrintNewCrashReport("Failed to start the rest service.");
      return EXIT_FAILURE;
    }
  #endif//TOKEN_ENABLE_REST_SERVICE

  #ifdef TOKEN_DEBUG
    LOG(INFO) << "Chain:";
    LOG(INFO) << " - Head: " << BlockChain::GetReference(BLOCKCHAIN_REFERENCE_HEAD);
    LOG(INFO) << " - Genesis: " << BlockChain::GetReference(BLOCKCHAIN_REFERENCE_GENESIS);

    ObjectPoolStats pool_stats = ObjectPool::GetStats();
    LOG(INFO) << "Pool Stats:";
    LOG(INFO) << " - Number of Objects: " << pool_stats.GetNumberOfObjects();
    LOG(INFO) << " - Number of Blocks: " << pool_stats.GetNumberOfBlocks();
    LOG(INFO) << " - Number of Transactions: " << pool_stats.GetNumberOfTransactions();
    LOG(INFO) << " - Number of Unclaimed Transactions: " << pool_stats.GetNumberOfUnclaimedTransactions();
  #endif//TOKEN_DEBUG

  AppendDummy(2);
  return EXIT_SUCCESS;
}