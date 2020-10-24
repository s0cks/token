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

    User user("VenueA");

    std::vector<Hash> utxos;
    if(!client->GetUnclaimedTransactions(user, utxos)){
        LOG(ERROR) << "cannot get unclaimed transactions for user: " << user;
        return EXIT_FAILURE;
    }

    intptr_t idx = 1;
    LOG(INFO) << "Unclaimed Transactions:";
    for(auto& it : utxos){
        Handle<UnclaimedTransaction> utxo = client->GetUnclaimedTransaction(it);
        if(it.IsNull()){
            LOG(WARNING) << (idx++) << ": Not Found";
        } else{
            LOG(INFO) << (idx++) << ": " << utxo;
        }
    }

    return EXIT_SUCCESS;
}