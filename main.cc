#include <sstream>
#include <string>
#include <fstream>
#include "blockchain.h"
#include "service/service.h"

static inline bool
FileExists(const std::string& name){
    std::ifstream f(name.c_str());
    return f.good();
}

int main(int argc, char** argv){
    using namespace Token;
    if(argc < 2) return EXIT_FAILURE;
    std::string path(argv[1]);
    if(!FileExists(path)) {
        std::cout << "Root path '" << path << "' doesn't exist" << std::endl;
        return EXIT_FAILURE;
    }
    BlockChain::GetInstance()->Load(path);
    BlockChainService::Start("127.0.0.1", 5051);
    BlockChainService::WaitForShutdown();
    return EXIT_SUCCESS;
}