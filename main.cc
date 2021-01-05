#include <thread>
#include "pool.h"
#include "keychain.h"
#include "discovery.h"
#include "blockchain.h"
#include "configuration.h"
#include "job/scheduler.h"
#include "utils/crash_report.h"

#include "utils/kvstore.h"

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

#include "job/verifier.h"

static inline void
InitializeLogging(char *arg0){
  using namespace Token;
  google::LogToStderr();
  google::InitGoogleLogging(arg0);
}

static inline void
PrintBanner(){
  // Print Debug Banner in Logs
  std::string header = "Token v" + Token::GetVersion() + " Debug Mode Enabled!";
  size_t total_size = 50;
  size_t middle = (total_size - header.size()) / 2;

  std::stringstream ss1;
  for(size_t idx = 0; idx < total_size; idx++) ss1 << "#";

  std::stringstream ss2;
  ss2 << "#";
  for(size_t idx = 0; idx < middle; idx++) ss2 << " ";
  ss2 << header;
  for(size_t idx = 0; idx < middle - 1; idx++) ss2 << " ";
  ss2 << "#";

  LOG(INFO) << ss1.str();
  LOG(INFO) << ss2.str();
  LOG(INFO) << ss1.str();
}

static inline bool
AppendDummy(){
  using namespace Token;
  sleep(5);
  LOG(INFO) << "getting unclaimed transactions";
  HashList utxos;
  if(!ObjectPool::GetUnclaimedTransactions(utxos)){
    LOG(ERROR) << "couldn't get unclaimed transactions for";
    return false;
  }
  LOG(INFO) << "spending " << utxos.size() << " unclaimed transactions";
  int64_t idx = 0;
  for(auto& it : utxos){
    LOG(INFO) << "spending token: " << it;
    UnclaimedTransactionPtr utxo = ObjectPool::GetUnclaimedTransaction(it);
    InputList inputs = {Input(utxo->GetTransaction(), utxo->GetIndex(), utxo->GetUser())};
    OutputList outputs = {Output("TestUser2", "TestToken2")};
    TransactionPtr tx = Transaction::NewInstance(idx++, inputs, outputs);

    ObjectPool::PutTransaction(tx->GetHash(), tx);


    sleep(2);
    if(idx == 2)
      break;
  }
  return true;
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

  #ifdef TOKEN_DEBUG
    PrintBanner();
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
    LOG(INFO) << "Number of Objects: " << ObjectPool::GetNumberOfObjects();
    LOG(INFO) << "Number of Blocks: " << ObjectPool::GetNumberOfBlocks();
    LOG(INFO) << "Number of Unclaimed Transactions: " << ObjectPool::GetNumberOfUnclaimedTransactions();
  #endif//TOKEN_DEBUG

//  if(!AppendDummy()){
//    CrashReport::PrintNewCrashReport("Cannot append dummy block.");
//    return EXIT_FAILURE;
//  }
  return EXIT_SUCCESS;
}