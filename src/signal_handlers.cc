#include "token.h"
#include "server.h"

namespace Token{
    void SignalHandlers::HandleInterrupt(int signum){
        LOG(INFO) << "terminating...";

        // 1. Disconnect Server
#ifdef TOKEN_DEBUG
        LOG(INFO) << "shutting down the server thread...";
#endif//TOKEN_DEBUG
        Server::Shutdown();

        //TODO: shutdown block discovery thread
        exit(0);
    }

    void SignalHandlers::HandleSegfault(int signnum){
        std::stringstream ss;
        ss << "Segfault";
        CrashReport::GenerateAndExit(ss.str());
    }
}