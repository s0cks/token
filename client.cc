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

    Allocator::Initialize();

    BlockChainClient* client = new BlockChainClient(NodeAddress(FLAGS_peer));
    client->Connect();

    PeerList peers;
    if(!client->GetPeers(peers)){
        LOG(WARNING) << "couldn't get a list of peers from the peer.";
        return EXIT_FAILURE;
    }

    LOG(INFO) << "Peers (" << peers.size() << "):";
    for(auto& peer : peers){
        LOG(INFO) << " - " << peer;
    }
    return EXIT_SUCCESS;
}