#include "blockchain.h"
#include "client.h"

int main(int argc, char** argv){
    using namespace Token;
    
    Client::Session::Initialize(5051, "127.0.0.1");
    return EXIT_SUCCESS;
}