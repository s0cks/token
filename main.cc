#include <sstream>
#include <string>
#include <glog/logging.h>
#include <sys/stat.h>
#include <gflags/gflags.h>

#include "allocator.h"
#include "blockchain.h"
#include "server.h"
#include "service/service.h"

static inline bool
FileExists(const std::string& name){
    std::ifstream f(name.c_str());
    return f.good();
}

static inline bool
InitializeLogging(char* arg0, const std::string path){
    if(!FileExists(path)){
        int rc;
        if((rc = mkdir(path.c_str(), S_IRWXU|S_IRWXG|S_IROTH|S_IXOTH)) == -1){
            std::cerr << "Couldn't initialize logging directory '" << path << "'..." << std::endl;
            return false;
        }
    }
    google::SetLogDestination(google::INFO, path.c_str());
    google::SetLogDestination(google::WARNING, path.c_str());
    google::SetLogDestination(google::ERROR, path.c_str());
    google::SetStderrLogging(google::INFO);
    google::InitGoogleLogging(arg0);
    return true;
}

// BlockChain flags
DEFINE_string(path, "blah", "The FS path for the BlockChain");

// RPC Service Flags
DEFINE_uint32(service_port, 0, "The port used for the RPC service");

// Server Flags
DEFINE_uint32(server_port, 0, "The port used for the BlockChain server");

int main(int argc, char** argv){
    using namespace Token;
    gflags::ParseCommandLineFlags(&argc, &argv, true);
    std::string logdir = (FLAGS_path + "/logs");
    if(!InitializeLogging(argv[0], logdir)){
        return EXIT_FAILURE;
    }

    /*
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
    */

    int* i = (int*)Allocator::Allocate(sizeof(int));
    int* j = (int*)Allocator::Allocate(sizeof(int));
    (*i) = 10;
    (*j) = 100;

    assert((*i) == 10);
    assert((*j) == 100);

    Allocator::PrintMinorHeap();
    Allocator::PrintMajorHeap();
    Allocator::AddReference(&i);
    Allocator::CollectMinor();

    int* k = (int*)Allocator::Allocate(sizeof(int));
    (*k) = 1000;

    assert((*j) == 100);
    assert((*k) == 1000);

    Allocator::PrintMinorHeap();
    Allocator::PrintMajorHeap();
    return EXIT_SUCCESS;
}