#include <sstream>
#include <string>
#include <alloc.h>
#include <heap.h>

static inline bool
FileExists(const std::string& name){
    std::ifstream f(name.c_str());
    return f.good();
}

class HeapPrinter : Token::HeapVisitor{
private:
    unsigned int counter_;
public:
    HeapPrinter():
        counter_(0){}
    ~HeapPrinter(){}

    bool VisitChunk(void* ptr, size_t size){
        std::cout << "  - #" << (counter_++) << " Chunk " << (*((int*)ptr)) << "; Size := " << size << std::endl;
        return true;
    }
};

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

    Heap heap(4096 * 32);
    int* i = reinterpret_cast<int*>(heap.Alloc(sizeof(int)));
    (*i) = 200;

    int* j = reinterpret_cast<int*>(heap.Alloc(sizeof(int)));
    (*j) = 300;

    std::cout << "i := " << (*i) << std::endl;
    std::cout << "j := " << (*j) << std::endl;

    HeapPrinter printer;
    heap.Accept(reinterpret_cast<HeapVisitor*>(&printer));
    return EXIT_SUCCESS;
}