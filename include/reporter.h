#ifndef TOKEN_REPORTER_H
#define TOKEN_REPORTER_H

#include <iostream>
#include "flags.h"

namespace Token{
    class CrashReporter{
    public:
        enum Flags{
            kNone = 1 << 0,
            kHeapInformation = 1 << 1,
            kBlockChainInformation = 1 << 2,
            kTransactionPoolInformation = 1 << 3,
            kUnclaimedTransactionPoolInformation = 1 << 4,
            kDetailed = 1 << 5,
            kCloseOnWrite = 1 << 6
        };
    private:
        std::fstream stream_;
        std::string message_;
        long flags_;
    public:
        CrashReporter(std::string message, long flags=kNone):
            stream_(std::fstream(GetFilename(), std::ios::out)),
            message_(message),
            flags_(flags|kCloseOnWrite){}
        ~CrashReporter();

        bool ShouldWriteHeapInformation(){
            return (flags_ & kHeapInformation) == kHeapInformation ||
                    IsDetailed();
        }

        bool ShouldWriteBlockChainInformation(){
            return (flags_ & kBlockChainInformation) == kBlockChainInformation ||
                    IsDetailed();
        }

        bool ShouldWriteTransactionPoolInformation(){
            return (flags_ & kTransactionPoolInformation) == kTransactionPoolInformation ||
                    IsDetailed();
        }

        bool ShouldWriteUnclaimedTransactionPoolInformation(){
            return (flags_ & kUnclaimedTransactionPoolInformation) == kUnclaimedTransactionPoolInformation ||
                    IsDetailed();
        }

        bool ShouldCloseOnWrite(){
            return (flags_ & kCloseOnWrite) == kCloseOnWrite;
        }

        bool IsDetailed(){
            return (flags_ & kDetailed) == kDetailed;
        }

        std::string GetMessage(){
            return message_;
        }

        std::string GetFilename(){
            return (TOKEN_BLOCKCHAIN_HOME + "/token-node-error.log");
        }

        std::ostream* GetStream(){
            return &stream_;
        }

        bool WriteReport();
    };
}

#endif //TOKEN_REPORTER_H