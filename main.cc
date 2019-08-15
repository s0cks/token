#include <sstream>
#include <string>
#include <service/service.h>
#include <alloc.h>
#include "blockchain.h"

static inline bool
FileExists(const std::string& name){
    std::ifstream f(name.c_str());
    return f.good();
}

// <local file storage path>
// <local listening port>
// <peer port>
int main(int argc, char** argv){
    using namespace Token;
    if(argc < 2) return EXIT_FAILURE;
    std::string path(argv[1]);
    if(!FileExists(path)) {
        std::cout << "Root path '" << path << "' doesn't exist" << std::endl;
        return EXIT_FAILURE;
    }
    int port = atoi(argv[2]);

    int pport = -1;
    if(argc > 3){
        pport = atoi(argv[3]);
    }

    /*
    if(!BlockChain::GetInstance()->Load(path)){
        std::cerr << "cannot load: " << path << std::endl;
        return EXIT_FAILURE;
    }

    std::cout << (*BlockChain::GetInstance()->GetHead()) << std::endl;
    BlockChainService::GetInstance()->Start("127.0.0.1", port + 1);
    BlockChain::GetServerInstance()->AddPeer("127.0.0.1", pport);
    BlockChain::GetInstance()->StartServer(port);
    while(BlockChain::GetServerInstance()->IsRunning());
    BlockChain::GetInstance()->WaitForServerShutdown();
    BlockChainService::GetInstance()->WaitForShutdown();
    */

    int* i = (int*) Allocator::Alloc(sizeof(int));
    Allocator::AddReference(reinterpret_cast<void**>(&i));
    (*i) = 10;
    int* j = (int*) Allocator::Alloc(sizeof(int));
    (*j) = 100;

    Allocator::GetInstance()->PrintMinor(std::cout);
    Allocator::GetInstance()->PrintMajor(std::cout);

    int* k = (int*) Allocator::Alloc(sizeof(int));
    (*k) = 1000;

    int* l = (int*) Allocator::Alloc(sizeof(int));
    (*l) = 10000;

    if(!Allocator::GetInstance()->CollectMinor()){
        std::cerr << "Couldn't collect minor" << std::endl;
        return EXIT_FAILURE;
    }

    std::cout << std::endl;
    Allocator::GetInstance()->PrintMinor(std::cout);
    Allocator::GetInstance()->PrintMajor(std::cout);
    std::cout << "i := " << (*i) << " - Should be live" << std::endl;
    std::cout << "j := " << (*j) << " - Shouldn't be live" << std::endl;
    std::cout << "k := " << (*k) << " - Should be live" << std::endl;
    std::cout << "l := " << (*l) << " - Shouldn't be live" << std::endl;
    std::cout << std::endl;

    return EXIT_SUCCESS;
}