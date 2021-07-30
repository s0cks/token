#include "flags.h"
#include "configuration.h"

#include "pool.h"
#include "runtime.h"

#include "node/node_server.h"
#include "task/task_engine.h"
#include "http/http_service_rest.h"
#include "http/http_service_health.h"

#include "peer/peer_session_manager.h"

/*#ifdef TOKEN_DEBUG
  static const std::vector<token::User> kOwners = {
    token::User("VenueA"),
    token::User("VenueB"),
    token::User("VenueC"),
  };
  static const std::vector<token::User> kRecipients = {
    token::User("TestUser1"),
    token::User("TestUser2"),
    token::User("TestUser3"),
    token::User("tazz"),
    token::User("chris"),
    token::User("adam"),
    token::User("george")
  };

  static inline token::User
  RandomUser(const std::vector<token::User>& users){
    static std::default_random_engine engine;
    static std::uniform_int_distribution<int> distribution(0, users.size()-1);
    return users[distribution(engine)];
  }

  static inline uint64_t
  RandomTotal(const uint64_t min, const uint64_t max){
    static std::default_random_engine engine;
    static std::uniform_int_distribution<uint64_t> distribution(min, max);
    return distribution(engine);
  }

  static inline bool
  SellTokens(const token::User& owner, uint64_t total){
//    DLOG(INFO) << owner << " spending " << total << " tokens....";
//    token::WalletManagerPtr wallets = token::WalletManager::GetInstance();
//    token::ObjectPoolPtr pool = token::ObjectPool::GetInstance();
//
//    token::Wallet wallet;
//    if(!wallets->GetWallet(owner, wallet)){
//      LOG(WARNING) << "couldn't get the wallet for: " << owner;
//      return false;
//    }
//
//    for(auto& it : wallet){
//      token::UnclaimedTransactionPtr token = pool->GetUnclaimedTransaction(it);
//      token::User recipient = RandomUser(kRecipients);
//
//      DLOG(INFO) << owner << " spending " << it << ".....";
//
//
//      token::InputList inputs = {
//          token::Input(token->GetReference(), owner),
//      };
//      token::OutputList outputs = {
//        token::Output(recipient, token->GetProduct()),
//      };
//      token::UnsignedTransactionPtr tx = token::UnsignedTransaction::NewInstance(token::Clock::now(), inputs, outputs);
//      if(!pool->PutUnsignedTransaction(tx->hash(), tx)){
//        LOG(WARNING) << "cannot add new transaction " << tx->ToString() << " to pool!";
//        return false;
//      }
//
//#ifdef TOKEN_ENABLE_EXPERIMENTAL
////      token::NodeAddress elastic = token::NodeAddress::ResolveAddress(FLAGS_elasticsearch_hostname);
////      token::SendEvent(elastic, token::elastic::SpendEvent(token::Clock::now(), owner, recipient, token));
//#endif//TOKEN_ENABLE_EXPERIMENTAL
//
//      if(--total <= 0)
//        break;
//    }
//    return true;
  }
#endif//TOKEN_DEBUG*/

template<class C, class T, bool fatal=false>
static inline void
SilentlyStartThread(){
  if(!T::Start() && fatal){
    LOG(FATAL) << "cannot start the " << C::GetThreadName() << " thread.";
    return;
  }
}

template<class Service, class ServiceThread, google::LogSeverity Severity=google::INFO>
static inline void
SilentlyStartService(ServiceThread& thread){
  if(!Service::IsEnabled()){
    DLOG(INFO) << "the " << Service::GetName() << " service is disabled, not starting.";
    return;
  }

  if(!thread.Start()){
    LOG_AT_LEVEL(Severity) << "couldn't start the " << Service::GetName() << " service on port: " << Service::GetPort();
    return;
  }
  DLOG(INFO) << "the " << Service::GetName() << " service has been started.";
}

template<class C, const google::LogSeverity& Severity=google::INFO>
static inline void
SilentlyInitialize(){
  if(!C::Initialize())
    LOG_AT_LEVEL(Severity) << "failed to initialize the " << C::GetName();
}

template<class Service, class ServiceThread, google::LogSeverity Severity=google::INFO>
static inline void
SilentlyWaitForShutdown(ServiceThread& thread){
  if(!Service::IsEnabled()){
    DLOG(INFO) << "the " << Service::GetName() << " service is disabled, not waiting for shutdown.";
    return;
  }

  DLOG(INFO) << "joining the " << Service::GetName() << " service....";
  if(!thread.Join()){
    LOG_AT_LEVEL(Severity) << "couldn't join the " << Service::GetName() << " service on port: " << Service::GetPort();
    return;
  }
  DLOG(INFO) << "the " << Service::GetName() << " service has been stopped.";
}

static inline void
PrintRuntimeInformation(){
//  using namespace token;
//  // print basic information
//  VLOG(0) << "home: " << TOKEN_BLOCKCHAIN_HOME;
//  VLOG(0) << "node-id: " << config::GetServerNodeID();
//  VLOG(0) << "current time: " << FormatTimestampReadable(Clock::now());
//
//  // print block chain information
//  BlockChainPtr chain = BlockChain::GetInstance();
//  VLOG(0) << "number of blocks in chain: " << chain->GetNumberOfBlocks();
//  VLOG(0) << "number of references in chain: " << chain->GetNumberOfReferences();
//  if(VLOG_IS_ON(2)){
//    VLOG(2) << "head: " << chain->GetHead()->ToString();
//    VLOG(2) << "genesis: " << chain->GetGenesis()->ToString();
//  } else if(VLOG_IS_ON(1)){
//    VLOG(1) << "head: " << chain->GetHeadHash();
//    VLOG(1) << "genesis: " << chain->GetGenesisHash();
//  }
//
//  // print object pool information
//  ObjectPoolPtr pool = ObjectPool::GetInstance();
//  VLOG(0) << "number of objects in pool: " << pool->GetNumberOfObjects();
//  VLOG(1) << "number of blocks in pool: " << pool->GetNumberOfBlocks();
//  VLOG(1) << "number of transactions in pool: " << pool->GetNumberOfUnsignedTransactions();
//  VLOG(1) << "number of unclaimed transactions in pool: " << pool->GetNumberOfUnclaimedTransactions();
//
//  // print peer information
//  PeerList peers;
//  DLOG_IF(WARNING, !config::GetPeerList(TOKEN_CONFIGURATION_NODE_PEERS, peers)) << "cannot get peer list.";
//  VLOG(0) << "number of peers: " << peers.size();
//  if(VLOG_IS_ON(1) && !peers.empty()){
//    VLOG(1) << "peers: ";
//    for(auto& it : peers)
//      VLOG(1) << " - " << it;
//  }
}

//TODO:
// - create global environment teardown and deconstruct routines
// - validity/consistency checks on block chain data
// - safer/better shutdown/terminate routines
int
main(int argc, char **argv){
  using namespace token;

  // Parse Command Line Arguments
  gflags::RegisterFlagValidator(&FLAGS_path, &ValidateFlagPath);
  gflags::RegisterFlagValidator(&FLAGS_num_workers, &ValidateFlagNumberOfWorkers);
  gflags::RegisterFlagValidator(&FLAGS_mining_interval, &ValidateFlagMiningInterval);
  gflags::RegisterFlagValidator(&FLAGS_remote, &ValidateFlagRemote);
  gflags::RegisterFlagValidator(&FLAGS_server_port, &ValidateFlagPort);
  gflags::RegisterFlagValidator(&FLAGS_num_peers, &ValidateFlagNumberOfPeers);
  gflags::RegisterFlagValidator(&FLAGS_healthcheck_port, &ValidateFlagPort);
  gflags::RegisterFlagValidator(&FLAGS_service_port, &ValidateFlagPort);
  gflags::ParseCommandLineFlags(&argc, &argv, true);

  // Initialize the Logging Framework
  google::LogToStderr();
  google::InitGoogleLogging(argv[0]);

  // Create the home directory if it doesn't exist
  if(!FileExists(TOKEN_BLOCKCHAIN_HOME)){
    if(!CreateDirectory(TOKEN_BLOCKCHAIN_HOME)){
      LOG(FATAL) << "cannot create ledger in: " << TOKEN_BLOCKCHAIN_HOME;
      return EXIT_FAILURE;
    }
  }

  auto blk = Block::Genesis();
  DLOG(INFO) << "transactions: ";
  for(auto& it : blk->transactions()){
    DLOG(INFO) << " - " << it->inputs().size() << ":" << it->outputs().size();
  }

  Block::Encoder encoder(blk.get(), codec::kDefaultEncoderFlags);
  auto data = internal::NewBufferFor(encoder);
  if(!encoder.Encode(data)){
    DLOG(FATAL) << "cannot encode block to buffer.";
    return EXIT_FAILURE;
  }

  data->SetReadPosition(0);
  data->SetWritePosition(0);
  Block::Decoder decoder(codec::kDefaultDecoderHints);

  auto result = decoder.Decode(data);
  DLOG(INFO) << "decoded: " << result->ToString();


  // initialize the configuration
  config::Initialize(FLAGS_path);

  Runtime runtime;

  // start the health check service
  http::HealthServiceThread health_service_thread;
  SilentlyStartService<http::HealthService, http::HealthServiceThread, google::FATAL>(health_service_thread);

  // initialize the keychain
  //TODO: SilentlyInitialize<Keychain, google::FATAL>();//TODO: refactor & parallelize

  // initialize the wallet manager
  //TODO: SilentlyInitialize<WalletManager, google::FATAL>();

  // initialize the block chain
  //TODO: SilentlyInitialize<BlockChain, google::FATAL>();
  // start the peer threads & connect to any known peers
  PeerSessionManager::Initialize(&runtime);

  // start the rest service
  http::RestService rest_service(uv_loop_new(), runtime.GetPool());
  ServerThread<http::RestService> rest_service_thread(&rest_service);
  SilentlyStartService<http::RestService, ServerThread<http::RestService>, google::FATAL>(rest_service_thread);

  runtime.Run();

//  //TODO: SilentlyWaitForShutdown<PeerSessionManager
//  SilentlyWaitForShutdown<http::RestService, http::RestServiceThread, google::FATAL>(rest_service_thread);
  SilentlyWaitForShutdown<http::HealthService, http::HealthServiceThread, google::FATAL>(health_service_thread);
  return EXIT_SUCCESS;
}