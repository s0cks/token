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

    NodeAddress address(FLAGS_peer);
    Handle<ClientSession> client = ClientSession::NewInstance(address);
    if(!client->Connect()){
        LOG(ERROR) << "couldn't connect to the peer: " << address;
        return EXIT_FAILURE;
    }

    std::vector<Hash> utxos;
    if(!client->GetUnclaimedTransactions(User("VenueA"), utxos)){
        LOG(ERROR) << "couldn't get the list of unclaimed transactions from the peer: " << address;
        return EXIT_FAILURE;
    }
    client->Disconnect();
    return EXIT_SUCCESS;
}