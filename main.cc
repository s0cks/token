#include "peer/peer_session_manager.h"#include "token.h"#include "common.h"#include "server.h"#include "async_task.h"#include "block_chain.h"#include "block_discovery.h"#include "configuration.h"#include "http/healthcheck.h"static inline voidInitializeLogging(char* arg0){    using namespace Token;    google::LogToStderr();    google::InitGoogleLogging(arg0);}static inline voidPrintBanner(){#ifdef TOKEN_DEBUG    // Print Debug Banner in Logs    std::string header = "Token v" + Token::GetVersion() + " Debug Mode Enabled!";    size_t total_size = 50;    size_t middle = (total_size - header.size()) / 2;    std::stringstream ss1;    for(size_t idx = 0; idx < total_size; idx++) ss1 << "#";    std::stringstream ss2;    ss2 << "#";    for(size_t idx = 0; idx < middle; idx++) ss2 << " ";    ss2 << header;    for(size_t idx = 0; idx < middle - 1; idx++) ss2 << " ";    ss2 << "#";    LOG(INFO) << ss1.str();    LOG(INFO) << ss2.str();    LOG(INFO) << ss1.str();#endif //TOKEN_DEBUG}static inline boolAppendDummy(){    using namespace Token;    sleep(5);    LOG(INFO) << "getting unclaimed transactions";    std::vector<Hash> utxos;    if(!UnclaimedTransactionPool::GetUnclaimedTransactions(utxos)){        LOG(ERROR) << "couldn't get unclaimed transactions for";        return false;    }    LOG(INFO) << "spending " << utxos.size() << " unclaimed transactions";    int64_t idx = 0;    for(auto& it : utxos){        LOG(INFO) << "spending token: " << it;        UnclaimedTransactionPtr utxo = UnclaimedTransactionPool::GetUnclaimedTransaction(it);        InputList inputs = {            Input(utxo->GetTransaction(), utxo->GetIndex(), utxo->GetUser())        };        OutputList outputs = {            Output("TestUser2", "TestToken2")        };        TransactionPtr tx = std::make_shared<Transaction>(idx++, inputs, outputs);        TransactionPool::PutTransaction(tx->GetHash(), tx);        sleep(2);        if(idx == 2) break;    }    return true;}//TODO:// - create global environment teardown and deconstruct routines// - validity/consistency checks on block chain data// - better merkle tree implementation// - need tech docs for points of interest// - cleanup logs// - more client commands?// - safer/better shutdown/terminate routinesintmain(int argc, char** argv){    using namespace Token;    // Install Signal Handlers    SignalHandlers::Initialize();    // Parse Command Line Arguments    gflags::ParseCommandLineFlags(&argc, &argv, true);    // Initialize the Logging Framework    InitializeLogging(argv[0]);    PrintBanner();    if(!BlockChainConfiguration::Initialize()){        LOG(ERROR) << "couldn't load the block chain configuration.";        return EXIT_FAILURE;    }    HealthCheckService::Initialize();    BlockChain::Initialize();    BlockDiscoveryThread::Start();    AsyncTaskThread::Start();    Server::Initialize();    PeerSessionManager::Initialize();    LOG(INFO) << "callback: " << BlockChainConfiguration::GetServerCallbackAddress();    LOG(INFO) << "localhost:8000 := " << NodeAddress::ResolveAddress("localhost:8000");    LOG(INFO) << "main-0 := " << NodeAddress::ResolveAddress("main-0:8080");    LOG(INFO) << "main.token-ledger:8080 := " << NodeAddress::ResolveAddress("main.token-ledger:8080");    LOG(INFO) << "main.token-ledger.svc.cluster.local:8080 := " << NodeAddress::ResolveAddress("main.token-ledger.svc.cluster.local:8080");    Server::WaitForState(Server::kStopped);    PeerSessionManager::Shutdown();    BlockDiscoveryThread::WaitForState(BlockDiscoveryThread::kStopped);    return EXIT_SUCCESS;}