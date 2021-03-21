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

  static inline void
  PrintStats(){
    using namespace token;
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
  }
#endif//TOKEN_DEBUG

static inline token::User
RandomUser(const std::vector<token::User>& users){
  static std::default_random_engine engine;
  static std::uniform_int_distribution<int> distribution(0, users.size());
  return users[distribution(engine)];
}

template<class C, bool fatal=false>
static inline void
SilentlyStartThread(const char* name){
  if(!C::Start()){
    DLOG(ERROR) << "cannot start the " << name << " thread.";
    if(fatal){
      std::stringstream cause;
      cause << "Cannot start the " << name << " thread.";
      token::CrashReport::PrintNewCrashReportAndExit(cause);
      return;
    }
  }

  DLOG(INFO) << "the " << name << " thread has been started.";
}

template<class C, bool fatal=false>
static inline void
SilentlyStartService(const char* name, const token::ServerPort& port){
  if(!IsValidPort(port))
    DLOG(INFO) << "not starting the " << name << " service.";

  if(!C::Start()){
    DLOG(ERROR) << "cannot start the " << name << " service on port: " << port;
    if(fatal){
      std::stringstream cause;
      cause << "Cannot start the " << name << " service on port: " << port;
      token::CrashReport::PrintNewCrashReportAndExit(cause);
    }
  }

  DLOG(INFO) << "the " << name << "service has been started on port: " << port;
}

template<class C, bool fatal=false>
static inline void
SilentlyInitialize(const char* name){
  if(!C::Initialize()){
    DLOG(ERROR) << "cannot initialize the " << name;
    if(fatal){
      std::stringstream cause;
      cause << "Failed to initialize the " << name;
      token::CrashReport::PrintNewCrashReportAndExit(cause);
    }
  }

  DLOG(INFO) << name << " initialized.";
}


template<class C, bool fatal=false>
static inline void
SilentlyWaitForShutdown(const char* name){
  if(!C::WaitForShutdown()){
    DLOG(ERROR) << "failed to wait for the " << name << " service to shutdown.";
    if(fatal){
      std::stringstream cause;
      cause << "Failed to wait for the " << name << " service to shutdown.";
      token::CrashReport::PrintNewCrashReportAndExit(cause);
    }
  }

  DLOG(INFO) << "the " << name << " service has shutdown.";
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
  SilentlyInitialize<ConfigurationManager>("configuration");
  // start the health check service
  SilentlyStartService<HttpHealthService>("healthcheck", FLAGS_healthcheck_port);
  // initialize the job scheduler
  SilentlyInitialize<JobScheduler>("job scheduler");
  // initialize the keychain
  SilentlyInitialize<Keychain>("keychain");
  // initialize the object pool
  SilentlyInitialize<ObjectPool>("object pool");
  // initialize the wallet manager
  SilentlyInitialize<WalletManager>("wallet manager");
  // initialize the block chain
  SilentlyInitialize<BlockChain>("block chain");
  // start the rpc server
  SilentlyStartService<LedgerServer>("server", FLAGS_server_port);
  // start the peer threads & connect to any known peers
  SilentlyInitialize<PeerSessionManager>("peer session manager");
  // start the miner thread
  SilentlyStartThread<BlockMinerThread>("miner");

#ifdef TOKEN_DEBUG
  sleep(5);
  PrintStats();

  if(FLAGS_append_test && !AppendDummy("VenueA", 2)){
    CrashReport::PrintNewCrashReport("Cannot append dummy transactions.");
    return EXIT_FAILURE;
  }
#endif//TOKEN_DEBUG

  SilentlyWaitForShutdown<PeerSessionManager>("peer session manager");
  SilentlyWaitForShutdown<LedgerServer>("server");
  SilentlyWaitForShutdown<HttpRestService>("rest");
  SilentlyWaitForShutdown<HttpHealthService>("healthcheck");
  return EXIT_SUCCESS;
}