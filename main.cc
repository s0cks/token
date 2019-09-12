#include <sstream>
#include <string>
#include <glog/logging.h>
#include <sys/stat.h>
#include <gflags/gflags.h>

#include "blockchain.h"
#include "server.h"
#include "service/service.h"

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

// BlockChain flags
DEFINE_string(root, "", "The FS path for the BlockChain");

// RPC Service Flags
DEFINE_uint32(service_port, 0, "The port used for the RPC service");

// Server Flags
DEFINE_uint32(server_port, 0, "The port used for the BlockChain server");

int main(int argc, char** argv){
    using namespace Token;
    if(!InitializeLogging(argv[0], FLAGS_root)){
        return EXIT_FAILURE;
    }
    gflags::ParseCommandLineFlags(&argc, &argv, true);

    std::string utxopool_path = (FLAGS_root + "/unclaimed.db");
    if(!UnclaimedTransactionPool::LoadUnclaimedTransactionPool(utxopool_path)){
        LOG(ERROR) << "Couldn't load unclaimed transaction pool from path: " << utxopool_path;
        return EXIT_FAILURE;
    }

    if(!BlockChain::Initialize(FLAGS_root)){
        LOG(ERROR) << "Couldn't load BlockChain from path: " << FLAGS_root;
        return EXIT_FAILURE;
    }

    if(FLAGS_server_port > 0){
        if(!BlockChainServer::Initialize(FLAGS_server_port)){
            LOG(ERROR) << "Couldn't initialize the BlockChain server";
            return EXIT_FAILURE;
        }
    }

    if(FLAGS_service_port > 0){
        BlockChainService::Start("0.0.0.0", FLAGS_service_port);
        LOG(INFO) << "BlockChainService started @ localhost:" << FLAGS_service_port;
        BlockChainService::WaitForShutdown();
    }

    if(FLAGS_server_port > 0){
        if(!BlockChainServer::ShutdownAndWait()){
            LOG(ERROR) << "Couldn't shutdown the BlockChain server";
            return EXIT_FAILURE;
        }
    }
    return EXIT_SUCCESS;
}