#include <sstream>
#include <string>
#include <fstream>
#include "node/node.h"

int main(int argc, char** argv){
    using namespace Token;
    int port = atoi(argv[1]);

    if(argc == 3){
        int pport = atoi(argv[2]);
        std::cout << "Connecting to peer: localhost:" << pport << std::endl;
        Node::Server::Initialize(port, "localhost", pport);
    } {
        Node::Server::Initialize(port);
    }
    return EXIT_SUCCESS;
}