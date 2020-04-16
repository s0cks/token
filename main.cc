#include <sstream>#include <string>#include <glog/logging.h>#include <sys/stat.h>#include "common.h"#include "keychain.h"#include "block_chain.h"#include "node/node.h"#include "service/service.h"#include "http/server.h"static inline boolFileExists(const std::string& name){    std::ifstream f(name.c_str());    return f.good();}static inline boolInitializeLogging(char* arg0){    std::string path = (TOKEN_BLOCKCHAIN_HOME + "/logs/");    if(!FileExists(path)){        int rc;        if((rc = mkdir(path.c_str(), S_IRWXU|S_IRWXG|S_IROTH|S_IXOTH)) == -1){            std::cerr << "Couldn't initialize logging directory '" << path << "'..." << std::endl;            return false;        }    }    google::SetLogDestination(google::INFO, path.c_str());    google::SetLogDestination(google::WARNING, path.c_str());    google::SetLogDestination(google::ERROR, path.c_str());    google::SetStderrLogging(google::INFO);    google::InitGoogleLogging(arg0);    return true;}bool GetPeers(const std::string str, std::vector<std::string>& peers){    std::stringstream ss(str);    std::string token;    while (std::getline(ss, token, ',')) {        peers.push_back(token);    }    return peers.size() > 0;}//TODO:// - organize startup process// - create global environment teardown and deconstruct routines// - validity/consistency checks on block chain data//TODO:// - allocator implementation// - better merkle tree implementation// - monitoring functionality?intmain(int argc, char** argv){    using namespace Token;    gflags::ParseCommandLineFlags(&argc, &argv, true);    if(!InitializeLogging(argv[0])){        fprintf(stderr, "Couldn't initialize logging!"); //TODO: Refactor        return EXIT_FAILURE;    }    if(!TokenKeychain::InitializeKeys()){        LOG(ERROR) << "couldn't initialize keychain";        return EXIT_FAILURE;    }    {        // Load the TransactionPool into memory        if(!TransactionPool::Initialize()){            LOG(ERROR) << "couldn't initialize the TransactionPool";            return EXIT_FAILURE;        }        if(TOKEN_VERBOSE){            //TODO: TransactionPoolPrinter::PrintAsInfo();        }    }    {        if(!UnclaimedTransactionPool::Initialize()){            LOG(ERROR) << "couldn't initialize the UnclaimedTransactionPool";            return EXIT_FAILURE;        }        if(TOKEN_VERBOSE){            //TODO: print        }    }    {        // Load the BlockChain into memory        if(!BlockChain::Initialize()){            LOG(ERROR) << "couldn't initialize the BlockChain";            return EXIT_FAILURE;        }        if(TOKEN_VERBOSE){            // TODO: BlockChainPrinter::PrintAsInfo(Printer::kDetailed);        }    }    //TODO: handle server + service start/stop better    /*    if(!BlockChainHttpServer::StartServer()){        LOG(ERROR) << "couldn't start http server...";        return EXIT_FAILURE;    }    */    if(FLAGS_port > 0){        if(!BlockChainServer::StartServer()){            LOG(ERROR) << "couldn't start BlockChain server";            return EXIT_FAILURE;        }        if(!FLAGS_peer_address.empty()) BlockChainServer::ConnectToPeer(FLAGS_peer_address, FLAGS_peer_port);    }    if(FLAGS_service_port > 0){        Token::BlockChainService::Start("localhost", FLAGS_service_port);        LOG(INFO) << "BlockChainService started @ localhost:" << FLAGS_service_port;        Token::BlockChainService::WaitForShutdown();    }    if(FLAGS_port > 0){        //TODO: shutdown block chain server + service    }    return EXIT_SUCCESS;}