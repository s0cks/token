#include "token.h"#include "common.h"#include "server.h"#include "async_task.h"#include "block_chain.h"#include "block_discovery.h"#include "transaction_pool.h"#include "unclaimed_transaction_pool.h"static inline voidInitializeLogging(char* arg0){    using namespace Token;    google::LogToStderr();    google::InitGoogleLogging(arg0);}static inline voidPrintBanner(){#ifdef TOKEN_DEBUG    // Print Debug Banner in Logs    std::string header = "Token v" + Token::GetVersion() + " Debug Mode Enabled!";    size_t total_size = 50;    size_t middle = (total_size - header.size()) / 2;    std::stringstream ss1;    for(size_t idx = 0; idx < total_size; idx++) ss1 << "#";    std::stringstream ss2;    ss2 << "#";    for(size_t idx = 0; idx < middle; idx++) ss2 << " ";    ss2 << header;    for(size_t idx = 0; idx < middle - 1; idx++) ss2 << " ";    ss2 << "#";    LOG(INFO) << ss1.str();    LOG(INFO) << ss2.str();    LOG(INFO) << ss1.str();#endif //TOKEN_DEBUG}//TODO:// - create global environment teardown and deconstruct routines// - validity/consistency checks on block chain data// - better merkle tree implementation// - need tech docs for points of interest// - cleanup logs// - more client commands?// - safer/better shutdown/terminate routinesintmain(int argc, char** argv){    using namespace Token;    // Install Signal Handlers    SignalHandlers::Initialize();    // Parse Command Line Arguments    gflags::ParseCommandLineFlags(&argc, &argv, true);    // Initialize the Logging Framework    InitializeLogging(argv[0]);    // Initialize the Allocator    Allocator::Initialize();    PrintBanner();    BlockChain::Initialize();    BlockDiscoveryThread::Start();    Server::Start();    AsyncTaskThread::Start();    sleep(5);    CrashReport::WriteNewCrashReport("This is a test");//------------------------------------------------------------------------------// Test Code/*    LOG(INFO) << "getting unclaimed transactions";    std::vector<uint256_t> utxos;    if(!UnclaimedTransactionPool::GetUnclaimedTransactions(utxos)){        LOG(ERROR) << "couldn't get unclaimed transactions for";        return EXIT_FAILURE;    }    LOG(INFO) << "spending " << utxos.size() << " unclaimed transactions";    uint32_t idx = 0;    for(auto& it : utxos){        LOG(INFO) << "spending token: " << it;        UnclaimedTransaction* utxo = UnclaimedTransactionPool::GetUnclaimedTransaction(it);        Input* inputs[1] = {            Input::NewInstance(utxo->GetTransaction(), utxo->GetIndex(), utxo->GetUser())        };        Output* outputs[1] = {            Output::NewInstance("TestUser2", "TestToken2")        };        Transaction* tx = Transaction::NewInstance(idx++, inputs, 1, outputs, 1);        TransactionPool::PutTransaction(tx);        if(idx == 2) break;    }*///----------------------------------------------------------------------------------    UnclaimedTransactionPool::Print();    Handle<Block> genesis = Block::Genesis();    Allocator::PrintNewHeap();    Allocator::MinorCollect();    Server::WaitForState(Server::kStopped);    BlockDiscoveryThread::WaitForState(BlockDiscoveryThread::kStopped);    return EXIT_SUCCESS;}