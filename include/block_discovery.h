#ifndef TOKEN_BLOCK_DISCOVERY_H
#define TOKEN_BLOCK_DISCOVERY_H

#include "vthread.h"

namespace Token{
    class BlockDiscoveryThread : public Thread{
    private:
        BlockDiscoveryThread() = delete;

        static void HandleThread(uword parameter);
    public:
        ~BlockDiscoveryThread() = delete;

        static bool Start(){
            //TODO: fix parameter
            return Thread::Start("BlockDiscoveryThread", &HandleThread, 0);
        }
    };
}

#endif //TOKEN_BLOCK_DISCOVERY_H