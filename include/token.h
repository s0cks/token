#ifndef TOKEN_TOKEN_H
#define TOKEN_TOKEN_H

#include <iostream>
#include <string>
#include <signal.h>
#include "crash_report.h"
#include "block_miner.h"

#define TOKEN_MAJOR_VERSION 1
#define TOKEN_MINOR_VERSION 0
#define TOKEN_REVISION_VERSION 0

namespace Token{
    std::string GetVersion();

    class SignalHandlers{
    private:
        static void HandleInterrupt(int signum);
        static void HandleSegfault(int signum);

        SignalHandlers() = delete;
    public:
        ~SignalHandlers() = delete;

        static void Initialize(){
            signal(SIGINT, &HandleInterrupt);
            signal(SIGSEGV, &HandleSegfault);
        }
    };
}

#endif //TOKEN_TOKEN_H