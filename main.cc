#include <sstream>
#include <string>
#include <glog/logging.h>
#include <sys/stat.h>
#include <block_queue.h>
#include <node/node.h>

#include "flags.h"
#include "allocator.h"
#include "key.h"
#include "blockchain.h"
#include "service/service.h"
#include "printer.h"

static inline bool
FileExists(const std::string& name){
    std::ifstream f(name.c_str());
    return f.good();
}

static inline bool
InitializeLogging(char* arg0){
    std::string path = (TOKEN_BLOCKCHAIN_HOME + "/logs/");
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

static inline bool
InitializeUnclaimedTransactionPool(){
    std::string path = (TOKEN_BLOCKCHAIN_HOME + "/unclaimed.db");
    return Token::UnclaimedTransactionPool::LoadUnclaimedTransactionPool(path);
}

int
main(int argc, char** argv){
    using namespace Token;
    gflags::ParseCommandLineFlags(&argc, &argv, true);
    if(!InitializeLogging(argv[0])){
        fprintf(stderr, "Couldn't initialize logging!");
        return EXIT_FAILURE;
    }

    if(!Allocator::Initialize(FLAGS_minheap_size, FLAGS_maxheap_size)){
        LOG(ERROR) << "couldn't initialize allocator";
        return EXIT_FAILURE;
    }

    if(!TokenKeychain::InitializeKeys()){
        LOG(ERROR) << "couldn't initial block chain keychain";
        return EXIT_FAILURE;
    }

    if(!InitializeUnclaimedTransactionPool()){
        LOG(ERROR) << "couldn't initialize UnclaimedTransactionPool";
        return EXIT_FAILURE;
    }

    if(!TransactionPool::Initialize(TOKEN_BLOCKCHAIN_HOME)){
        LOG(ERROR) << "couldn't initialize the transaction pool";
        return EXIT_FAILURE;
    }

    if(!BlockChain::Initialize()){
        LOG(ERROR) << "couldn't initialize the blockchain";
        return EXIT_FAILURE;
    }

    if(TOKEN_VERBOSE){
        UnclaimedTransactionPoolPrinter::Print();
        BlockPrinter::PrintAsInfo(BlockChain::GetHead(), true);
    }

    if(FLAGS_service_port > 0){
        BlockChainService::Start("127.0.0.1", FLAGS_service_port);
        LOG(INFO) << "BlockChainService started @ localhost:" << FLAGS_service_port;
        BlockChainService::WaitForShutdown();
    }
    return EXIT_SUCCESS;
}