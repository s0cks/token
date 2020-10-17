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

    Allocator::Initialize();

/*
ClientSession* client = new ClientSession(true);
client->Connect(NodeAddress(FLAGS_peer_address, FLAGS_peer_port));
client->WaitForState(ClientSession::State::kDisconnected);
*/

    BlockChainClient* client = new BlockChainClient(NodeAddress(FLAGS_peer_address, FLAGS_peer_port));
    client->Connect();

    LOG(INFO) << "Head: " << client->GetHead();

    uint256_t hash = client->GetHead().GetHash();
    Handle<Block> block = client->GetBlock(hash);
    LOG(INFO) << "Block: " << block;
    return EXIT_SUCCESS;
}