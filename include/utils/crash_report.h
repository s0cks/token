#ifndef TOKEN_CRASH_REPORT_H
#define TOKEN_CRASH_REPORT_H

#include <cstdlib>
#include <fstream>
#include "common.h"

#ifdef OS_IS_LINUX
#include <execinfo.h>
#include <cxxabi.h>
#endif//OS_IS_LINUX

namespace Token{
#ifdef OS_IS_LINUX
    class StackTrace{
    public:
        static const int32_t kDefaultStackTraceDepth = 20;
    private:
        void** frames_;
        int32_t frames_len_;
        char** symbols_;
    public:
        StackTrace(int32_t offset=0, int32_t depth=StackTrace::kDefaultStackTraceDepth):
            frames_(nullptr),
            frames_len_(0){
            int32_t total_size = offset + depth;
            if(!(frames_ = (void**)malloc(sizeof(void*)*total_size))){
                fprintf(stderr, "couldn't allocate stack trace of size: %" PRId32 "\n", total_size);
                return;
            }

            if((frames_len_ = backtrace(frames_, total_size)) == 0){
                fprintf(stderr, "couldn't get stack trace of size: %" PRId32 "\n", total_size);
                return;
            }

            if(!(symbols_ = backtrace_symbols(frames_, frames_len_))){
                fprintf(stderr, "couldn't get symbols for stack trace of size: %" PRId32 "\n", total_size);
                return;
            }
        }
        ~StackTrace() = default;

        int32_t GetNumberOfFrames() const{
            return frames_len_;
        }

        char* operator[](int32_t idx){
            return symbols_[idx];
        }

        char* operator[](int32_t idx) const{
            return symbols_[idx];
        }
    };
#endif//OS_IS_LINUX

    class CrashReport{
    private:
        std::string filename_;
        std::string cause_;
#ifdef OS_IS_LINUX
        StackTrace trace_;

        CrashReport(const std::string& filename, const std::string& cause):
            filename_(filename),
            cause_(cause),
            trace_(){}
#else
        CrashReport(const std::string& filename, const std::string& cause):
            filename_(filename),
            cause_(cause){}
#endif//OS_IS_LINUX

        static inline std::string
        GetNewCrashReportFilename(){
            std::stringstream filename;
            filename << CrashReport::GetCrashReportDirectory();
            filename << "/crash-report-" << GetTimestampFormattedFileSafe(GetCurrentTimestamp()) << ".log";
            return filename.str();
        }
    public:
        ~CrashReport() = default;

        std::string GetFilename() const{
            return filename_;
        }

        std::string GetCause() const{
            return cause_;
        }

        static bool PrintNewCrashReport(const std::string& cause, const google::LogSeverity& severity=google::INFO);
        static bool PrintNewCrashReportAndExit(const std::string& cause, const google::LogSeverity& severity=google::INFO, int code=EXIT_FAILURE);

        static inline bool
        PrintNewCrashReport(const std::stringstream& ss, const google::LogSeverity& severity=google::INFO){
            return PrintNewCrashReport(ss.str(), severity);
        }

        static inline bool
        PrintNewCrashReportAndExit(const std::stringstream& ss, const google::LogSeverity& severity=google::INFO, int code=EXIT_FAILURE){
            return PrintNewCrashReportAndExit(ss.str(), severity, code);
        }

        static bool WriteNewCrashReport(const std::string& cause);
        static void WriteNewCrashReportAndExit(const std::string& cause);

        static inline bool
        WriteNewCrashReport(const std::stringstream& ss){
            return WriteNewCrashReport(ss.str());
        }

        static inline void
        WriteNewCrashReportAndExit(const std::stringstream& ss){
            return WriteNewCrashReportAndExit(ss.str());
        }

        static inline std::string
        GetCrashReportDirectory(){
            return TOKEN_BLOCKCHAIN_HOME;
        }
    };
}

#endif //TOKEN_CRASH_REPORT_H