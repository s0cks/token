#include "token.h"
#include "server.h"

namespace Token{
    void SignalHandlers::HandleInterrupt(int signum){
        LOG(INFO) << "terminating...";
        //TODO:
        // - shutdown server thread
        // - shutdown proposer thread
        // - shutdown block discovery thread

        uv_loop_t* loop = uv_default_loop();
        uv_stop(loop);

        exit(0);
    }

    void SignalHandlers::HandleSegfault(int signnum){
        std::stringstream ss;
        ss << "Segfault";
        CrashReport::GenerateAndExit(ss.str());
    }
}