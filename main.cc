#include "node/node.h"#include "node/session.h"#include "common.h"#include "crash_report.h"#include "block_chain.h"#include "block_miner.h"#ifdef TOKEN_HEALTHCHECK_SUPPORT#include "http/server.h"#endif//TOKEN_HEALTHCHECK_SUPPORTstatic inline voidInitializeLogging(char* arg0){    using namespace Token;    if(!FileExists(FLAGS_path)){        if(!CreateDirectory(FLAGS_path)){            std::stringstream ss;            ss << "Couldn't initialize the block chain directory: " << FLAGS_path;            CrashReport::GenerateAndExit(ss);        }    }    std::string path = (TOKEN_BLOCKCHAIN_HOME + "/logs/");    if(!FileExists(path)){        if(!CreateDirectory(path)){            std::stringstream ss;            ss << "Couldn't initialize the logging directory: " << path;            CrashReport::GenerateAndExit(ss);        }    }    google::SetLogDestination(google::INFO, path.c_str());    google::SetLogDestination(google::WARNING, path.c_str());    google::SetLogDestination(google::ERROR, path.c_str());    google::SetStderrLogging(google::INFO);    google::InitGoogleLogging(arg0);}//TODO:// - organize startup process// - create global environment teardown and deconstruct routines// - validity/consistency checks on block chain data//TODO:// - better merkle tree implementation// - monitoring functionality?intmain(int argc, char** argv){    using namespace Token;    // 1. Install Signal Handlers    SignalHandlers::Initialize();    // 2. Parse Command Line Arguments    gflags::ParseCommandLineFlags(&argc, &argv, true);    // 3. Initialize Logging    InitializeLogging(argv[0]);#ifdef TOKEN_DEBUG    // Print Debug Banner in Logs    std::string header = "Token Debug Mode Enabled!";    size_t total_size = 50;    size_t middle = (total_size - header.size()) / 2;    std::stringstream ss1;    for(size_t idx = 0; idx < total_size; idx++) ss1 << "#";    std::stringstream ss2;    ss2 << "#";    for(size_t idx = 0; idx < middle; idx++) ss2 << " ";    ss2 << header;    for(size_t idx = 0; idx < middle - 1; idx++) ss2 << " ";    ss2 << "#";    LOG(INFO) << ss1.str();    LOG(INFO) << ss2.str();    LOG(INFO) << ss1.str();#endif //TOKEN_DEBUG    // 4. Initialize the Block Chain State    BlockChain::Initialize();    // 5. Start the Block Miner Thread    BlockMiner::Initialize();    // 6. Optionally start the Block Chain Server Thread    if(FLAGS_port > 0) Node::Start();    if(FLAGS_peer_port > 0){        if(!Node::ConnectTo(FLAGS_peer_address, FLAGS_peer_port)){            std::stringstream ss;            ss << "Couldn't connect to peer: " << FLAGS_peer_address << ":" << FLAGS_peer_port;            CrashReport::GenerateAndExit(ss);        }    }    Allocator::Collect();    Allocator::PrintAllocated();    // 7. Optionally start the Health Check Server Thread#ifdef TOKEN_HEALTHCHECK_SUPPORT    BlockChainHttpServer::StartServer();#endif//TOKEN_HEALTHCHECK_SUPPORT    /*    //TODO: initialize node info    //TODO: need tech docs for points of interest    sleep(10);    std::vector<uint256_t> utxos;    if(!UnclaimedTransactionPool::GetUnclaimedTransactions(utxos)){        LOG(ERROR) << "couldn't get unclaimed transactions for: TestUser";        return EXIT_FAILURE;    }    Token::UnclaimedTransaction* utxo1;    if(!(utxo1 = UnclaimedTransactionPool::GetUnclaimedTransaction(utxos[0]))){        LOG(ERROR) << "couldn't get unclaimed transaction: " << utxos[0];        return EXIT_FAILURE;    }    Token::UnclaimedTransaction* utxo2;    if(!(utxo2 = UnclaimedTransactionPool::GetUnclaimedTransaction(utxos[1]))){        LOG(ERROR) << "couldn't get unclaimed transaction: " << utxos[1];        return EXIT_FAILURE;    }    Transaction::InputList tx1_inputs = {        Input(utxo1->GetTransaction(), utxo1->GetIndex(), utxo1->GetUser()),    };    Transaction::OutputList tx1_outputs = {        Output("TestUser", "TestToken"),    };    Transaction* tx1 = Transaction::NewInstance(0, tx1_inputs, tx1_outputs);    TransactionPool::PutTransaction(tx1);    Transaction::InputList tx2_inputs = {        Input(utxo2->GetTransaction(), utxo2->GetIndex(), utxo2->GetUser()),    };    Transaction::OutputList tx2_outputs = {        Output("TestUser", "TestToken"),    };    Transaction* tx2 = Transaction::NewInstance(1, tx2_inputs, tx2_outputs);    TransactionPool::PutTransaction(tx2);    CrashReport::GenerateAndExit("This is a test");     */    // 8. Shutdown the Block Chain Server Thread    Node::WaitForShutdown();    // 9. Shutdown the Block Miner Thread    BlockMiner::WaitForStoppedState();    return EXIT_SUCCESS;}