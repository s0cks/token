#include <sstream>
#include <string>
#include <fstream>
#include "node/node.h"

int main(int argc, char** argv){
    using namespace Token;
    
    Node::Server::Initialize(5051);
    return EXIT_SUCCESS;
}