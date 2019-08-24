#include <sstream>
#include <string>
#include <glog/logging.h>
#include <sys/stat.h>
#include <service/service.h>
#include "blockchain.h"
#include "server.h"

static inline bool
FileExists(const std::string& name){
    std::ifstream f(name.c_str());
    return f.good();
}

static inline bool
InitializeLogging(char* arg0, const std::string& path){
    std::string logdir = (path + "/logs/");
    if(!FileExists(logdir)){
        int rc;
        if((rc = mkdir(logdir.c_str(), S_IRWXU|S_IRWXG|S_IROTH|S_IXOTH)) == -1){
            std::cerr << "Couldn't initialize logging directory '" << logdir << "'..." << std::endl;
            return false;
        }
    }
    google::SetLogDestination(google::INFO, logdir.c_str());
    google::SetLogDestination(google::WARNING, logdir.c_str());
    google::SetLogDestination(google::ERROR, logdir.c_str());
    google::SetStderrLogging(google::INFO);
    google::InitGoogleLogging(arg0);
    return true;
}

// <local file storage path>
// <local listening port>
// <peer port>
int main(int argc, char** argv){
    using namespace Token;
    if(argc < 2) return EXIT_FAILURE;
    std::string path(argv[1]);
    if(!FileExists(path)) {
        std::cout << "Root path '" << path << "' doesn't exist" << std::endl;
        return EXIT_FAILURE;
    }
    int port = atoi(argv[2]);

    int pport = -1;
    if(argc > 3){
        pport = atoi(argv[3]);
    }

    if(!InitializeLogging(argv[0], path)){
        return EXIT_FAILURE;
    }

    if(!UnclaimedTransactionPool::LoadUnclaimedTransactionPool((path + "/unclaimed.db"))){
        LOG(ERROR) << "Cannot load unclaimed transaction pool from path '" << (path + "/unclaimed.db") << "'";
        return EXIT_FAILURE;
    }
    UnclaimedTransactionPoolPrinter::Print();

    if(!BlockChain::Initialize(path)){
        return EXIT_FAILURE;
    }

    if(pport > 0){
        BlockChainServer::AddPeer("127.0.0.1", pport);
    }
    if(!BlockChainServer::Initialize(port)){
        LOG(ERROR) << "Couldn't initialize the BlockChain server";
        return EXIT_FAILURE;
    }

    BlockChainService::Start("0.0.0.0", port + 1);
    LOG(INFO) << "BlockChainService started @ localhost:" << (port + 1);
    BlockChainService::WaitForShutdown();

    if(!BlockChainServer::ShutdownAndWait()){
        LOG(ERROR) << "couldn't shutdown the server";
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}