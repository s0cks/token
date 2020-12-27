#include <thread>#include "hash.h"#include "common.h"#include "server.h"#include "blockchain.h"#include "discovery.h"#include "configuration.h"#include "pool.h"#include "http/rest_service.h"#include "http/healthcheck_service.h"#include "peer/peer_session_manager.h"#include "rapidjson/filewritestream.h"#include "rapidjson/prettywriter.h"#include "rapidjson/ostreamwrapper.h"static inline voidInitializeLogging(char* arg0){    using namespace Token;    google::LogToStderr();    google::InitGoogleLogging(arg0);}static inline voidPrintBanner(){    // Print Debug Banner in Logs    std::string header = "Token v" + Token::GetVersion() + " Debug Mode Enabled!";    size_t total_size = 50;    size_t middle = (total_size - header.size()) / 2;    std::stringstream ss1;    for(size_t idx = 0; idx < total_size; idx++) ss1 << "#";    std::stringstream ss2;    ss2 << "#";    for(size_t idx = 0; idx < middle; idx++) ss2 << " ";    ss2 << header;    for(size_t idx = 0; idx < middle - 1; idx++) ss2 << " ";    ss2 << "#";    LOG(INFO) << ss1.str();    LOG(INFO) << ss2.str();    LOG(INFO) << ss1.str();}static inline boolAppendDummy(){    using namespace Token;    sleep(5);    LOG(INFO) << "getting unclaimed transactions";    HashList utxos;    if(!ObjectPool::GetUnclaimedTransactions(utxos)){        LOG(ERROR) << "couldn't get unclaimed transactions for";        return false;    }    LOG(INFO) << "spending " << utxos.size() << " unclaimed transactions";    int64_t idx = 0;    for(auto& it : utxos){        LOG(INFO) << "spending token: " << it;        UnclaimedTransactionPtr utxo = ObjectPool::GetUnclaimedTransaction(it);        InputList inputs = {            Input(utxo->GetTransaction(), utxo->GetIndex(), utxo->GetUser())        };        OutputList outputs = {            Output("TestUser2", "TestToken2")        };        TransactionPtr tx = std::make_shared<Transaction>(idx++, inputs, outputs);        ObjectPool::PutObject(tx->GetHash(), tx);        sleep(2);        if(idx == 2) break;    }    return true;}class MyOStreamWrapper{private:    Token::Buffer* buff_;public:    typedef char Ch;    MyOStreamWrapper(Token::Buffer* buff):        buff_(buff){}    Ch Peek() const{ return '\0'; }    Ch Take(){ return '\0'; }    size_t Tell() const{        return buff_->GetWrittenBytes();    }    Ch* PutBegin(){        return 0;    }    void Put(Ch c){        buff_->PutByte(c);    }    void Flush(){}    size_t PutEnd(){        return 0;    }};//TODO:// - create global environment teardown and deconstruct routines// - validity/consistency checks on block chain data// - better merkle tree implementation// - need tech docs for points of interest// - cleanup logs// - more client commands?// - safer/better shutdown/terminate routinesintmain(int argc, char** argv){    using namespace Token;    // Install Signal Handlers    SignalHandlers::Initialize();    // Parse Command Line Arguments    gflags::ParseCommandLineFlags(&argc, &argv, true);    // Initialize the Logging Framework    InitializeLogging(argv[0]);#ifdef TOKEN_DEBUG    PrintBanner();#endif//TOKEN_DEBUG    if(!BlockChainConfiguration::Initialize()){        LOG(ERROR) << "couldn't load the block chain configuration.";        return EXIT_FAILURE;    }    //JobScheduler::Initialize();    HealthCheckService::Start();    BlockChain::Initialize();    RestService::Start();    Server::Start();    PeerSessionManager::Initialize();    BlockDiscoveryThread::Start();    BlockPtr blk = Block::Genesis();    ObjectPool::PutObject(blk->GetHash(), blk);    LOG(INFO) << "Number of Objects: " << ObjectPool::GetNumberOfObjects();    LOG(INFO) << "Number of Blocks: " << ObjectPool::GetNumberOfBlocks();    LOG(INFO) << "Number of Unclaimed Transactions: " << ObjectPool::GetNumberOfUnclaimedTransactions();    return EXIT_SUCCESS;}