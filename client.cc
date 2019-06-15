#include "blockchain.h"
#include "client.h"
#include <unistd.h>

int main(int argc, char** argv){
    using namespace Token;
    Client* client = Client::Connect("127.0.0.1", atoi(argv[1]), [](Client* instance){

    });
    return EXIT_SUCCESS;
}