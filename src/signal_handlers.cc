#include "token.h"
#include "node/node.h"

namespace Token{
    void SignalHandlers::HandleInterrupt(int signum){
        LOG(INFO) << "terminating...";

        // 1. Shutdown Server
#ifdef TOKEN_DEBUG
        LOG(INFO) << "shutting down the server thread...";
#endif//TOKEN_DEBUG
        Node::Shutdown();

        // 2. Shutdown Block Miner
#ifdef TOKEN_DEBUG
        LOG(INFO) << "shutting down the miner thread...";
#endif//TOKEN_DEBUG
        BlockMiner::Shutdown();
        exit(0);
    }

    void SignalHandlers::HandleSegfault(int signnum){
        std::stringstream ss;
        ss << "Segfault";
        CrashReport::GenerateAndExit(ss.str());
    }
}