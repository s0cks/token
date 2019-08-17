#include <sstream>
#include <string>
#include <service/service.h>
#include "blockchain.h"
#include <leveldb/db.h>
#include <glog/logging.h>
#include <sys/stat.h>

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

    leveldb::DB* db;
    leveldb::Options options;
    options.create_if_missing = true;

    leveldb::Status status = leveldb::DB::Open(options, (path + "/state.db"), &db);
    if(!status.ok()){
        LOG(ERROR) << "Cannot open state.db";
        return EXIT_FAILURE;
    }

    leveldb::WriteOptions writeOpts;
    db->Put(writeOpts, "Height", "1");

    leveldb::ReadOptions readOpts;

    std::string value;
    if(!db->Get(readOpts, "Height", &value).ok()){
        LOG(ERROR) << "Cannot get height";
        return EXIT_FAILURE;
    }

    LOG(INFO) << "Height: " << value;

    /*
    if(!BlockChain::GetInstance()->Load(path)){
        LOG(ERROR) << "Cannot load BlockChain from path '" << path << "'" << std::endl;
        return EXIT_FAILURE;
    }
    BlockChainService::GetInstance()->Start("127.0.0.1", port + 1);
    BlockChain::GetServerInstance()->AddPeer("127.0.0.1", pport);
    BlockChain::GetInstance()->StartServer(port);
    while(BlockChain::GetServerInstance()->IsRunning());
    BlockChain::GetInstance()->WaitForServerShutdown();
    BlockChainService::GetInstance()->WaitForShutdown();
    */
    return EXIT_SUCCESS;
}