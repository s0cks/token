#include "common.h"
#include "client.h"

static inline bool
InitializeLogging(char *arg0){
  google::SetStderrLogging(google::INFO);
  google::InitGoogleLogging(arg0);
  return true;
}

static inline bool
SpendToken(Token::ClientSession *client,
           const Token::UnclaimedTransactionPtr &utxo,
           const std::string &user,
           const std::string &product){
  Token::InputList inputs = {
      Token::Input(utxo->GetTransaction(), utxo->GetIndex(), utxo->GetUser()),
  };
  Token::OutputList outputs = {
      Token::Output(user, product),
  };

  Token::TransactionPtr tx = std::make_shared<Token::Transaction>(0, inputs, outputs);
  client->SendTransaction(tx);
  return true;
}

int
main(int argc, char **argv){
  using namespace Token;
  gflags::ParseCommandLineFlags(&argc, &argv, true);
  if(!InitializeLogging(argv[0])){
    fprintf(stderr, "Couldn't initialize logging!");
    return EXIT_FAILURE;
  }

  if(FLAGS_peer.empty()){
    LOG(WARNING) << "please specify a peer address using --peer";
    return EXIT_FAILURE;
  }

  LOG(INFO) << "peer: " << FLAGS_peer;

  NodeAddress address(FLAGS_peer);
  ClientSession *client = new ClientSession(address);
  if(!client->Connect()){
    LOG(ERROR) << "couldn't connect to the peer: " << address;
    return EXIT_FAILURE;
  }

  client->Disconnect();
  return EXIT_SUCCESS;
}