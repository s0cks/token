#include <sstream>#include <string>#include <glog/logging.h>#include <sys/stat.h>#include "node/node.h"#include "node/session.h"#include "common.h"#include "keychain.h"#include "block_chain.h"#include "block_miner.h"static inline boolFileExists(const std::string& name){    std::ifstream f(name.c_str());    return f.good();}static inline boolInitializeLogging(char* arg0){    std::string path = (TOKEN_BLOCKCHAIN_HOME + "/logs/");    if(!FileExists(path)){        int rc;        if((rc = mkdir(path.c_str(), S_IRWXU|S_IRWXG|S_IROTH|S_IXOTH)) == -1){            std::cerr << "Couldn't initialize logging directory '" << path << "'..." << std::endl;            return false;        }    }    google::SetLogDestination(google::INFO, path.c_str());    google::SetLogDestination(google::WARNING, path.c_str());    google::SetLogDestination(google::ERROR, path.c_str());    google::SetStderrLogging(google::INFO);    google::InitGoogleLogging(arg0);    return true;}bool GetPeers(const std::string str, std::vector<std::string>& peers){    std::stringstream ss(str);    std::string token;    while (std::getline(ss, token, ',')) {        peers.push_back(token);    }    return peers.size() > 0;}//TODO:// - organize startup process// - create global environment teardown and deconstruct routines// - validity/consistency checks on block chain data//TODO:// - allocator implementation// - better merkle tree implementation// - monitoring functionality?intmain(int argc, char** argv){    using namespace Token;    gflags::ParseCommandLineFlags(&argc, &argv, true);    if(!InitializeLogging(argv[0])){        fprintf(stderr, "Couldn't initialize logging!"); //TODO: Refactor        return EXIT_FAILURE;    }    if(!TokenKeychain::InitializeKeys()){        LOG(ERROR) << "couldn't initialize keychain";        return EXIT_FAILURE;    }    {        // Load the TransactionPool into memory        if(!TransactionPool::Initialize()){            LOG(ERROR) << "couldn't initialize the TransactionPool";            return EXIT_FAILURE;        }    }    {        // Load the UnclaimedTransactionPool into memory        if(!UnclaimedTransactionPool::Initialize()){            LOG(ERROR) << "couldn't initialize the UnclaimedTransactionPool";            return EXIT_FAILURE;        }    }    {        // Load the BlockPool into memory        if(!BlockPool::Initialize()){            LOG(ERROR) << "couldn't initialize the BlockPool";            return EXIT_FAILURE;        }    }    {        // Load the BlockChain into memory        if (!BlockChain::Initialize()) {            LOG(ERROR) << "couldn't initialize the BlockChain";            return EXIT_FAILURE;        }        if(!BlockMiner::Initialize()){            LOG(ERROR) << "couldn't start block miner thread";            return EXIT_FAILURE;        }    }    if(FLAGS_port > 0){        if(!Node::Start()){            LOG(ERROR) << "couldn't start BlockChain server";            return EXIT_FAILURE;        }        if(!FLAGS_peer_address.empty()){            NodeAddress paddr(FLAGS_peer_address, FLAGS_peer_port);            if(!Node::ConnectTo(paddr)){                LOG(ERROR) << "couldn't connect to peer: " << paddr;                return EXIT_FAILURE;            }        }    }    /*    //TODO: need tech docs for points of interest    sleep(20);    std::vector<uint256_t> utxos;    if(!UnclaimedTransactionPool::GetUnclaimedTransactions(utxos)){        LOG(ERROR) << "couldn't get unclaimed transactions for: TestUser";        return EXIT_FAILURE;    }    Token::UnclaimedTransaction* utxo1;    if(!(utxo1 = UnclaimedTransactionPool::GetUnclaimedTransaction(utxos[0]))){        LOG(ERROR) << "couldn't get unclaimed transaction: " << utxos[0];        return EXIT_FAILURE;    }    Token::UnclaimedTransaction* utxo2;    if(!(utxo2 = UnclaimedTransactionPool::GetUnclaimedTransaction(utxos[1]))){        LOG(ERROR) << "couldn't get unclaimed transaction: " << utxos[1];        return EXIT_FAILURE;    }    Transaction::InputList tx1_inputs = {            std::unique_ptr<Input>(Input::NewInstance(utxo1->GetTransaction(), utxo1->GetIndex()))    };    Transaction::OutputList tx1_outputs = {            std::unique_ptr<Output>(Output::NewInstance("TestUser", "TestToken"))    };    Transaction* tx1 = Transaction::NewInstance(0, tx1_inputs, tx1_outputs);    if(!TransactionPool::AddTransaction(tx1)){        LOG(WARNING) << "couldn't put transaction #1 into tx pool: " << tx1->GetHash();        return EXIT_FAILURE;    }    Transaction::InputList tx2_inputs = {            std::unique_ptr<Input>(Input::NewInstance(utxo2->GetTransaction(), utxo2->GetIndex()))    };    Transaction::OutputList tx2_outputs = {            std::unique_ptr<Output>(Output::NewInstance("TestUser", "TestToken"))    };    Transaction* tx2 = Transaction::NewInstance(1, tx2_inputs, tx2_outputs);    if(!TransactionPool::AddTransaction(tx2)){        LOG(WARNING) << "couldn't put transaction #2 into tx pool: " << tx2->GetHash();        return EXIT_FAILURE;    }    //TODO: handle server + rpc start/stop better    /*    if(!BlockChainHttpServer::StartServer()){        LOG(ERROR) << "couldn't start http server...";        return EXIT_FAILURE;    }    */    sleep(5);    if(FLAGS_port > 0) Node::WaitForShutdown();    return EXIT_SUCCESS;}