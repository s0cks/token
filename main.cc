#include <sstream>
#include <string>
#include <glog/logging.h>
#include <sys/stat.h>
#include <gflags/gflags.h>

#include "allocator.h"
#include "blockchain.h"
#include "server.h"
#include "service/service.h"

// BlockChain flags
DEFINE_string(path, "", "The FS path for the BlockChain");
DEFINE_uint32(minheap_size, 128 * 20, "The size of the minor heap");
DEFINE_uint32(maxheap_size, 128 * 32 * 5, "The size of the major heap");

// RPC Service Flags
DEFINE_uint32(service_port, 0, "The port used for the RPC service");

// Server Flags
DEFINE_uint32(server_port, 0, "The port used for the BlockChain server");
DEFINE_uint32(peer_port, 0, "The port to connect to a peer with");

static inline bool
FileExists(const std::string& name){
    std::ifstream f(name.c_str());
    return f.good();
}

static inline bool
InitializeLogging(char* arg0){
    std::string path = (FLAGS_path + "/logs/");
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
    std::string path = (FLAGS_path + "/unclaimed.db");
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

    if(!InitializeUnclaimedTransactionPool()){
        LOG(ERROR) << "couldn't initialize UnclaimedTransactionPool";
        return EXIT_FAILURE;
    }

    if(!TransactionPool::Initialize(FLAGS_path)){
        LOG(ERROR) << "couldn't initialize the transaction pool";
        return EXIT_FAILURE;
    }

    if(!BlockChain::Initialize(FLAGS_path)){
        return EXIT_FAILURE;
    }

    BlockChainPrinter::PrintBlockChain();

    if(FLAGS_server_port > 0){
        if(!BlockChainServer::Initialize(FLAGS_server_port)){
            LOG(ERROR) << "Couldn't initialize the BlockChain server";
            return EXIT_FAILURE;
        }
    }

    if(FLAGS_server_port > 0 && FLAGS_peer_port > 0){
        if(!BlockChainServer::ConnectToPeer("127.0.0.1", FLAGS_peer_port)){
            LOG(ERROR) << "Couldn't connect to peer on port: " << FLAGS_peer_port;
            return EXIT_FAILURE;
        }
    }

    LOG(INFO) << "Min Chunk Size: " << Allocator::kMinChunkSize;
    LOG(INFO) << "Transaction Size: " << (sizeof(Transaction) + sizeof(int));
    LOG(INFO) << "Block Size :" << (sizeof(Block) + sizeof(int));

    UnclaimedTransactionPoolPrinter::Print();

    BlockChainPrinter::PrintBlockChain(true);

    Allocator::PrintMinorHeap();
    Allocator::PrintMajorHeap();

    Block* head = BlockChain::GetHead();
    Transaction* tx1 = new Transaction();
    tx1->AddInput(head->GetCoinbaseTransaction()->GetHash(), 0);
    tx1->AddOutput("Token0", "TestUser5");
    if(!TransactionPool::AddTransaction(tx1)){
        LOG(ERROR) << "Couldn't add new transaction to tx pool";
        return EXIT_FAILURE;
    }

    Block* nhead;
    if(!(nhead = TransactionPool::CreateBlock())){
        LOG(ERROR) << "Couldn't create new block from tx pool";
        return EXIT_FAILURE;
    }

    if(!BlockChain::AppendBlock(nhead)){
        LOG(ERROR) << "Couldn't append new head to block chain";
        return EXIT_FAILURE;
    }

    Allocator::PrintMinorHeap();
    Allocator::PrintMajorHeap();

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