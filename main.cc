#include <thread>
#include "pool.h"
#include "miner.h"
#include "wallet.h"
#include "keychain.h"
#include "blockchain.h"
#include "configuration.h"
#include "job/scheduler.h"

#include "crash/crash_report.h"

// --path "/usr/share/ledger"
DEFINE_string(path, "", "The path for the local ledger to be stored in.");
// --enable-snapshots
DEFINE_bool(enable_snapshots, false, "Enable snapshots of the block chain");
// --num-workers 10
DEFINE_int32(num_workers, 4, "Define the number of worker pool threads");
// --miner-interval 3600
DEFINE_int64(miner_interval, 1000 * 60 * 1, "The amount of time between mining blocks in milliseconds.");

#ifdef TOKEN_DEBUG
  // --append-test
  DEFINE_bool(append_test, false, "Append a test block upon startup [Debug]");
#endif//TOKEN_DEBUG

#ifdef TOKEN_ENABLE_SERVER
  #include "server/rpc/rpc_server.h"
  #include "peer/peer_session_manager.h"

  // --remote localhost:8080
  DEFINE_string(remote, "", "The hostname for the remote ledger to synchronize with.");
  // --server-port 8080
  DEFINE_int32(server_port, 0, "The port for the ledger RPC service.");
  // --num-peers 12
  DEFINE_int32(num_peers, 4, "The max number of peers to connect to.");
#endif//TOKEN_ENABLE_SERVER

#ifdef TOKEN_ENABLE_HEALTH_SERVICE
  #include "http/health_service.h"

  // --healthcheck-port 8081
  DEFINE_int32(healthcheck_port, 0, "The port for the ledger health check service.");
#endif//TOKEN_ENABLE_HEALTHCHECK

#ifdef TOKEN_ENABLE_REST_SERVICE
  #include "http/rest_service.h"

  // --service-port 8082
  DEFINE_int32(service_port, 0, "The port for the ledger controller service.");
#endif//TOKEN_ENABLE_REST_SERVICE

static inline void
InitializeLogging(char *arg0){
  using namespace Token;
  google::LogToStderr();
  google::InitGoogleLogging(arg0);
}

#ifdef TOKEN_DEBUG
  static inline bool
  AppendDummy(int total_spends){
    using namespace Token;
    sleep(10);

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
      InputList inputs = {};
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

  class BlockChainPrinter : public Token::BlockChainBlockVisitor, Token::Printer{
   public:
    BlockChainPrinter(const google::LogSeverity& severity, const long& flags):
      Token::BlockChainBlockVisitor(),
      Token::Printer(severity, flags){}
    ~BlockChainPrinter() = default;

    bool Visit(const Token::BlockPtr& blk){
      LOG_AT_LEVEL(GetSeverity()) << blk->GetHash();
      return true;
    }

    static bool Print(const google::LogSeverity& severity=google::INFO, const long& flags=Printer::kFlagNone){
      BlockChainPrinter printer(severity, flags);
      return Token::BlockChain::VisitBlocks(&printer);
    }
  };
#endif//TOKEN_DEBUG

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
    LOG(INFO) << "current time: " << FormatTimestampReadable(Clock::now());
  #endif//TOKEN_DEBUG

  // Load the configuration
  if(!BlockChainConfiguration::Initialize()){
    CrashReport::PrintNewCrashReport("Failed to load the configuration.");
    return EXIT_FAILURE;
  }

  // Start the health service if enabled
  #ifdef TOKEN_ENABLE_HEALTH_SERVICE
    if(IsValidPort(FLAGS_healthcheck_port) && !HttpHealthService::Start()){
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

  // Start the server if enabled
  #ifdef TOKEN_ENABLE_SERVER
    if(IsValidPort(FLAGS_server_port) && !LedgerServer::Start()){
      CrashReport::PrintNewCrashReport("Failed to start the server.");
      return EXIT_FAILURE;
    }

    if(!PeerSessionManager::Initialize()){
      CrashReport::PrintNewCrashReport("Failed to initialize the peer session manager.");
      return EXIT_FAILURE;
    }
  #endif//TOKEN_ENABLE_SERVER

  // Start the controller service if enabled
  #ifdef TOKEN_ENABLE_REST_SERVICE
    if(IsValidPort(FLAGS_service_port) && !HttpRestService::Start()){
      CrashReport::PrintNewCrashReport("Failed to start the controller service.");
      return EXIT_FAILURE;
    }
  #endif//TOKEN_ENABLE_REST_SERVICE

#ifdef TOKEN_DEBUG
  if(FLAGS_append_test && !AppendDummy(Block::kMaxTransactionsForBlock)){
    CrashReport::PrintNewCrashReport("Cannot append dummy transactions.");
    return EXIT_FAILURE;
  }
#endif//TOKEN_DEBUG

  if(!BlockMiner::Start()){
    CrashReport::PrintNewCrashReport("Cannot start the block miner.");
    return EXIT_FAILURE;
  }

#ifdef TOKEN_ENABLE_SERVER
  if(IsValidPort(FLAGS_server_port) && LedgerServer::IsServerRunning() && !LedgerServer::WaitForShutdown()){
    CrashReport::PrintNewCrashReport("Cannot join the server thread.");
    return EXIT_FAILURE;
  }
#endif//TOKEN_ENABLE_SERVER

#ifdef TOKEN_ENABLE_REST_SERVICE
  if(HttpRestService::IsServiceRunning() && !HttpRestService::WaitForShutdown()){
    CrashReport::PrintNewCrashReport("Cannot join the controller service thread.");
    return EXIT_FAILURE;
  }
#endif//TOKEN_ENABLE_REST_SERVICE

#ifdef TOKEN_ENABLE_HEALTH_SERVICE
  if(HttpHealthService::IsServiceRunning() && !HttpHealthService::WaitForShutdown()){
    CrashReport::PrintNewCrashReport("Cannot join the health check service thread.");
    return EXIT_FAILURE;
  }
#endif//TOKEN_ENABLE_HEALTH_SERVICE
  return EXIT_SUCCESS;
}