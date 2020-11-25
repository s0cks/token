#include "common.h"
#include "client.h"

static inline bool
InitializeLogging(char* arg0){
    google::SetStderrLogging(google::INFO);
    google::InitGoogleLogging(arg0);
    return true;
}

int
main(int argc, char** argv){
    using namespace Token;
    gflags::ParseCommandLineFlags(&argc, &argv, true);
    if(!InitializeLogging(argv[0])){
        fprintf(stderr, "Couldn't initialize logging!"); //TODO: Refactor
        return EXIT_FAILURE;
    }

    if(FLAGS_peer.empty()){
        LOG(WARNING) << "please specify a peer address using --peer";
        return EXIT_FAILURE;
    }

    if(!BlockChainClient::Initialize()){
        LOG(ERROR) << "couldn't initialize the client.";
        return EXIT_FAILURE;
    }

    NodeAddress address(FLAGS_peer);
    BlockChainClient* client = new BlockChainClient(address);
    if(!client->Connect()){
        LOG(ERROR) << "couldn't connect to the peer: " << address;
        return EXIT_FAILURE;
    }

    std::set<Hash> blocks;
    if(!client->GetBlockChain(blocks)){
        LOG(ERROR) << "couldn't get the list of blocks from the peer: " << address;
        return EXIT_FAILURE;
    }

    for(auto& it : blocks){
        LOG(INFO) << " - " << it;
    }

    client->Disconnect();
    client->WaitForDisconnect();
    return EXIT_SUCCESS;
}