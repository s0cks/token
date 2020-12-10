#include "common.h"
#include "client.h"

static inline bool
InitializeLogging(char* arg0){
    google::SetStderrLogging(google::INFO);
    google::InitGoogleLogging(arg0);
    return true;
}

static inline bool
SpendToken(Token::ClientSession* client, Token::UnclaimedTransaction* utxo, const std::string& user, const std::string& product){
    Token::InputList inputs = {
        Token::Input(utxo->GetTransaction(), utxo->GetIndex(), utxo->GetUser()),
    };
    Token::OutputList outputs = {
        Token::Output(user, product),
    };
    client->SendTransaction(Token::Transaction(0, inputs, outputs));
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

    LOG(INFO) << "peer: " << FLAGS_peer;

    NodeAddress address(FLAGS_peer);
    ClientSession* client = new ClientSession(address);
    if(!client->Connect()){
        LOG(ERROR) << "couldn't connect to the peer: " << address;
        return EXIT_FAILURE;
    }

    std::vector<Hash> utxos;
    if(!client->GetUnclaimedTransactions(User("VenueA"), utxos)){
        LOG(ERROR) << "couldn't get the list of unclaimed transactions from the peer: " << address;
        return EXIT_FAILURE;
    }

    for(int32_t idx = 0;
        idx < 2; idx++){
        UnclaimedTransaction* utxo = client->GetUnclaimedTransaction(utxos[idx]);
        SpendToken(client, utxo, "TestUser2", "TestToken");
    }
    client->Disconnect();
    return EXIT_SUCCESS;
}