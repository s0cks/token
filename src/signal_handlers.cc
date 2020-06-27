#include "token.h"
#include "node/node.h"

namespace Token{
    void SignalHandlers::HandleInterrupt(int signum){
        LOG(INFO) << "Shutting down.....";
        //if(!BlockMiner::Shutdown()) CrashReport::GenerateAndExit("Couldn't shutdown Block Miner");
        if(!Node::Shutdown()) CrashReport::GenerateAndExit("Couldn't shutdown the server");
        exit(0);
    }

    void SignalHandlers::HandleSegfault(int signnum){
        std::stringstream ss;
        ss << "Segfault";
        CrashReport::GenerateAndExit(ss.str());
    }
}