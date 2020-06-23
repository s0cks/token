#ifndef TOKEN_CRASH_REPORT_H
#define TOKEN_CRASH_REPORT_H

#include <fstream>
#include "common.h"

namespace Token{
    class CrashReport{
    public:
        enum{
            kIncludeNone,
            kIncludeStacktrace = 1 << 1,
            kIncludeMemory = 1 << 2,
            kIncludeBlockChain = 1 << 3,
            kIncludeTransactionPool = 1 << 4,
            kIncludeBlockPool = 1 << 5,
            kIncludeUnclaimedTransactionPool = 1 << 6,
            kIncludeAll=kIncludeStacktrace|kIncludeMemory|kIncludeBlockChain|kIncludeTransactionPool|kIncludeBlockPool|kIncludeUnclaimedTransactionPool
        };
    private:
        std::fstream file_;
        std::string message_;
        long flags_;

        inline std::fstream&
        GetStream(){
            return file_;
        }

        long GetFlags() const{
            return flags_;
        }

        CrashReport(const std::string& filename, const std::string& msg, long flags):
            file_(filename, std::ios::out),
            message_(msg),
            flags_(flags){}
    public:
        std::string GetMessage() const{
            return message_;
        }

        bool ShouldIncludeStacktrace() const{
            return (GetFlags() & kIncludeStacktrace) == kIncludeStacktrace;
        }

        bool ShouldIncludeMemory() const{
            return (GetFlags() & kIncludeMemory) == kIncludeMemory;
        }

        bool ShouldIncludeBlockChain() const{
            return (GetFlags() & kIncludeBlockChain) == kIncludeBlockChain;
        }

        bool ShouldIncludeTransactionPool() const{
            return (GetFlags() & kIncludeTransactionPool) == kIncludeTransactionPool;
        }

        bool ShouldIncludeBlockPool() const{
            return (GetFlags() & kIncludeBlockPool) == kIncludeBlockPool;
        }

        bool ShouldIncludeUnclaimedTransactionPool() const{
            return (GetFlags() & kIncludeUnclaimedTransactionPool) == kIncludeUnclaimedTransactionPool;
        }

        inline bool
        WriteLine(const std::string& line){
            GetStream() << line << std::endl;
            return true;
        }

        inline bool
        WriteLine(const std::stringstream& line){
            GetStream() << line.str() << std::endl;
            return true;
        }

        inline bool
        WriteNewline(){
            GetStream() << std::endl;
            return true;
        }

        friend CrashReport& operator<<(CrashReport& stream, const std::string& line){
            stream.GetStream() << line << std::endl;
            return stream;
        }

        friend CrashReport& operator<<(CrashReport& stream, const std::stringstream& line){
            stream.GetStream() << line.str() << std::endl;
            return stream;
        }

        friend CrashReport& operator<<(CrashReport& stream, const bool& value){
            stream.GetStream() << value;
            return stream;
        }

        static bool Generate(const std::string& msg, long flags=kIncludeAll);
        static int GenerateAndExit(const std::string& msg, long flags=kIncludeAll);
    };
}

#endif //TOKEN_CRASH_REPORT_H