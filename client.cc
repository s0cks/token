#include "common.h"
#include "node/session.h"

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

    NodeClient* client = new NodeClient();
    client->Connect(NodeAddress(FLAGS_peer_address, FLAGS_peer_port));
    client->WaitForShutdown();
    return EXIT_SUCCESS;
}