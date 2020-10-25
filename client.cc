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

    NodeAddress address;
    BlockChainClient* client = new BlockChainClient(address);
    if(!client->Connect()){
        LOG(ERROR) << "couldn't connect to the peer: " << address;
        return EXIT_FAILURE;
    }

    PeerList peers;
    if(!client->GetPeers(peers)){
        LOG(WARNING) << "couldn't get peer list from peer.";
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}