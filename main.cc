#include "pool.h"
#include "units.h"
#include "flags.h"
#include "miner.h"
#include "wallet.h"
#include "keychain.h"
#include "reference.h"
#include "filesystem.h"
#include "blockchain.h"
#include "configuration.h"
#include "job/scheduler.h"


#ifdef TOKEN_ENABLE_EXPERIMENTAL
#include "elastic/elastic_client.h"
#include "elastic/events/elastic_spend_event.h"
#endif//TOKEN_ENABLE_EXPERIMENTAL

#include "network/rpc_server.h"
#include "network/peer_session_manager.h"
#include "http/http_service_health.h"
#include "http/http_service_rest.h"

#ifdef TOKEN_DEBUG
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
    DLOG(INFO) << owner << " spending " << total << " tokens....";
    token::WalletManagerPtr wallets = token::WalletManager::GetInstance();
    token::ObjectPoolPtr pool = token::ObjectPool::GetInstance();

    token::Wallet wallet;
    if(!wallets->GetWallet(owner, wallet)){
      LOG(WARNING) << "couldn't get the wallet for: " << owner;
      return false;
    }

    for(auto& it : wallet){
      token::UnclaimedTransactionPtr token = pool->GetUnclaimedTransaction(it);
      token::User recipient = RandomUser(kRecipients);

      DLOG(INFO) << owner << " spending " << it << ".....";


      token::InputList inputs = {
          token::Input(token->GetReference(), owner),
      };
      token::OutputList outputs = {
        token::Output(recipient, token->GetProduct()),
      };
      token::TransactionPtr tx = token::Transaction::NewInstance(inputs, outputs);
      if(!pool->PutTransaction(tx->hash(), tx)){
        LOG(WARNING) << "cannot add new transaction " << tx->ToString() << " to pool!";
        return false;
      }

      token::NodeAddress elastic = token::NodeAddress::ResolveAddress(FLAGS_elasticsearch_hostname);
      token::SendEvent(elastic, token::elastic::SpendEvent(token::Clock::now(), owner, recipient, token));

      if(--total <= 0)
        break;
    }
    return true;
  }
#endif//TOKEN_DEBUG

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

template<class Component, const google::LogSeverity& Severity=google::INFO>
static inline void
SilentlyInitialize(){
  DLOG(INFO) << "initializing the " << Component::GetName() << "....";
  if(!Component::Initialize()){
    LOG_AT_LEVEL(Severity) << "couldn't initialize the " << Component::GetName();
    return;
  }
  DLOG(INFO) << "the " << Component::GetName() << " has been initialized.";
}

template<class C, bool fatal=false>
static inline void
SilentlyInitialize(){
  if(!C::Initialize() && fatal){
    LOG(FATAL) << "failed to initialize the " << C::GetName();
    return;
  }
}

template<class Service, class ServiceThread, google::LogSeverity Severity=google::INFO>
static inline void
SilentlyWaitForShutdown(ServiceThread& thread){
  if(!Service::IsEnabled()){
    DLOG(INFO) << "the " << Service::GetName() << " service is disabled, not waiting for shutdown.";
    return;
  }

  if(!thread.Join()){
    LOG_AT_LEVEL(Severity) << "couldn't join the " << Service::GetName() << " service on port: " << Service::GetPort();
    return;
  }
  DLOG(INFO) << "the " << Service::GetName() << " service has been stopped.";
}

static inline void
PrintRuntimeInformation(){
  using namespace token;
  // print basic information
  VLOG(0) << "home: " << TOKEN_BLOCKCHAIN_HOME;
  VLOG(0) << "node-id: " << ConfigurationManager::GetNodeID();
  VLOG(0) << "current time: " << FormatTimestampReadable(Clock::now());

  // print block chain information
  BlockChainPtr chain = BlockChain::GetInstance();
  VLOG(0) << "number of blocks in chain: " << chain->GetNumberOfBlocks();
  VLOG(0) << "number of references in chain: " << chain->GetNumberOfReferences();
  if(VLOG_IS_ON(2)){
    VLOG(2) << "head: " << chain->GetHead()->ToString();
    VLOG(2) << "genesis: " << chain->GetGenesis()->ToString();
  } else if(VLOG_IS_ON(1)){
    VLOG(1) << "head: " << chain->GetHead()->ToString();
    VLOG(1) << "genesis: " << chain->GetGenesis()->ToString();
  }

  // print object pool information
  ObjectPoolPtr pool = ObjectPool::GetInstance();
  VLOG(0) << "number of objects in pool: " << pool->GetNumberOfObjects();
  VLOG(1) << "number of blocks in pool: " << pool->GetNumberOfBlocks();
  VLOG(1) << "number of transactions in pool: " << pool->GetNumberOfTransactions();
  VLOG(1) << "number of unclaimed transactions in pool: " << pool->GetNumberOfUnclaimedTransactions();

  // print peer information
  PeerList peers;
  DLOG_IF(WARNING, !ConfigurationManager::GetInstance()->GetPeerList(TOKEN_CONFIGURATION_NODE_PEERS, peers)) << "cannot get peer list.";
  VLOG(0) << "number of peers: " << peers.size();
  if(VLOG_IS_ON(1) && !peers.empty()){
    VLOG(1) << "peers: ";
    for(auto& it : peers)
      VLOG(1) << " - " << it;
  }
}

static token::rpc::LedgerServerThread server_thread;
static token::http::HealthServiceThread health_service_thread;
static token::http::RestServiceThread rest_service_thread;

static void
AllocBuffer(uv_handle_t* handle, size_t suggested_size, uv_buf_t* buf){
  LOG(INFO) << "allocating buffer of size " << suggested_size << "b for session";
  buf->base = (char*)malloc(suggested_size);
  buf->len = suggested_size;
}

static void
OnMessageSent(uv_write_t* req, int status){
  LOG(INFO) << "status: " << status;
}

static void
OnClose(uv_handle_t* handle){
  LOG(INFO) << "session closed.";
}

static void
OnRead(uv_stream_t* stream, ssize_t nread, const uv_buf_t* buff){
  token::http::ResponsePtr response = token::http::NewOkResponse("Hello World");
  token::BufferPtr buffer = response->ToBuffer();

  LOG(INFO) << "response: " << std::string(buffer->data(), buffer->GetWrittenBytes());

  uv_buf_t buffers[1];
  buffers[0].base = buffer->data();
  buffers[0].len = buffer->GetWrittenBytes();

  uv_write_t* request = new uv_write_t();
  uv_write(request, stream, buffers, 1, &OnMessageSent);

  uv_read_stop(stream);
  uv_close((uv_handle_t*)stream, &OnClose);
}

static void
OnNewConnection(uv_stream_t* stream, int status){
  assert(status == 0);

  uv_tcp_t* session = new uv_tcp_t();
  uv_tcp_init(stream->loop, session);
  uv_accept(stream, (uv_stream_t*)session);
  uv_read_start((uv_stream_t*)session, &AllocBuffer, &OnRead);
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
//  if(!FileExists(TOKEN_BLOCKCHAIN_HOME)){
//    if(!CreateDirectory(TOKEN_BLOCKCHAIN_HOME)){
//      LOG(FATAL) << "cannot create ledger in: " << TOKEN_BLOCKCHAIN_HOME;
//      return EXIT_FAILURE;
//    }
//  }

/*uv_loop_t* loop = uv_loop_new();
  uv_tcp_t server;
  uv_tcp_init(loop, &server);

  sockaddr_in bind_address{};
  uv_ip4_addr("0.0.0.0", FLAGS_service_port, &bind_address);
  uv_tcp_bind(&server, (struct sockaddr*)&bind_address, 0);
  uv_listen((uv_stream_t*)&server, 100, &OnNewConnection);

  uv_run(loop, UV_RUN_DEFAULT);*/

  http::HealthService service;
  service.Run(FLAGS_service_port);

//  // Load the configuration
//  SilentlyInitialize<ConfigurationManager, google::FATAL>();
//  // start the health check service
//  SilentlyStartService<http::HealthService, http::HealthServiceThread, google::FATAL>(health_service_thread);
//  // initialize the job scheduler
//  SilentlyInitialize<JobScheduler, google::FATAL>();
//  // initialize the keychain
//  SilentlyInitialize<Keychain, google::FATAL>();//TODO: refactor & parallelize
//  // initialize the ReferenceDatabase
//  SilentlyInitialize<ReferenceDatabase, google::FATAL>();
//  // initialize the object pool
//  SilentlyInitialize<ObjectPool, google::FATAL>();
//  // initialize the wallet manager
//  SilentlyInitialize<WalletManager, google::FATAL>();
//  // initialize the block chain
//  SilentlyInitialize<BlockChain, google::FATAL>();
//  // start the rpc server
//  SilentlyStartService<rpc::LedgerServer, rpc::LedgerServerThread, google::FATAL>(server_thread);
//  // start the peer threads & connect to any known peers
//  SilentlyInitialize<PeerSessionManager, google::FATAL>();
//  // start the rest service
//  SilentlyStartService<http::RestService, http::RestServiceThread, google::FATAL>(rest_service_thread);
//
//  if(FLAGS_mining_interval > 0)
//    SilentlyStartThread<BlockMiner, BlockMinerThread>();
//
//  sleep(5);
//  PrintRuntimeInformation();

#ifdef TOKEN_DEBUG
  sleep(2);
  if(FLAGS_spend_test_min >= 0 && FLAGS_spend_test_max > 0){
    for(auto& owner : kOwners){
      SellTokens(owner, RandomTotal(FLAGS_spend_test_min, FLAGS_spend_test_max));
    }
  }
#endif//TOKEN_DEBUG

//  //TODO: SilentlyWaitForShutdown<PeerSessionManager
//  SilentlyWaitForShutdown<rpc::LedgerServer, rpc::LedgerServerThread, google::FATAL>(server_thread);
//  SilentlyWaitForShutdown<http::RestService, http::RestServiceThread, google::FATAL>(rest_service_thread);
//  SilentlyWaitForShutdown<http::HealthService, http::HealthServiceThread, google::FATAL>(health_service_thread);
  return EXIT_SUCCESS;
}