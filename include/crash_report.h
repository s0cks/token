#ifndef TOKEN_CRASH_REPORT_H
#define TOKEN_CRASH_REPORT_H

#include <string>
#include <fstream>
#include <time.h>
#include "common.h"

namespace Token{
    class CrashReport{
    public:
        static const int kBannerSize = 64;

        enum Flags{
            kNone = 1 << 0,
            kAllocatorInfo = 1 << 1,
            kBlockChainInfo = 1 << 2,
            kTransactionPoolInfo = 1 << 3,
            kUnclaimedTransactionPoolInfo = 1 << 4,
            kAll = kAllocatorInfo|kBlockChainInfo|kTransactionPoolInfo|kUnclaimedTransactionPoolInfo
        };
    private:
        std::fstream file_;
        std::string filename_;
        std::string cause_;
        uint32_t flags_;

        void PrintAllocatorInformation();
        void PrintBlockChainInformation();
        void PrintTransactionPoolInformation();
        void PrintUnclaimedTransactionPoolInformation();

        static std::string CreateNewFilename(){
            std::stringstream stream;
            stream << FLAGS_path << "/crash-";
            time_t raw_time;
            time(&raw_time);
            struct tm* timeinfo = localtime(&raw_time);
            char buffer[256];
            strftime(buffer, sizeof(buffer), "%Y%m%d-%H%M%S", timeinfo);
            stream << buffer << ".log";
            return stream.str();
        }
    public:
        CrashReport(const std::string& cause, const std::string& filename=CreateNewFilename(), uint32_t flags=kAll):
            file_(filename, std::ios::out),
            filename_(filename),
            cause_(cause),
            flags_(flags){}
        ~CrashReport(){}

        std::string GetFilename() const{
            return filename_;
        }

        std::string GetCause() const{
            return cause_;
        }

        bool ShouldPrintAllocatorInformation() const{
            return (flags_ & kAllocatorInfo) == kAllocatorInfo;
        }

        bool ShouldPrintBlockChainInformation() const{
            return (flags_ & kBlockChainInfo) == kBlockChainInfo;
        }

        bool ShouldPrintTransactionPoolInformation() const{
            return (flags_ & kTransactionPoolInfo) == kTransactionPoolInfo;
        }

        bool ShouldPrintUnclaimedTransactionPoolInformation() const{
            return (flags_ & kUnclaimedTransactionPoolInfo) == kUnclaimedTransactionPoolInfo;
        }

        int WriteReport();
    };
}

#endif //TOKEN_CRASH_REPORT_H