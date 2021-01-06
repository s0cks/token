#ifndef TOKEN_CRASH_REPORT_H
#define TOKEN_CRASH_REPORT_H

#include <cstdlib>
#include <fstream>
#include "common.h"
#include "printer.h"

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
    StackTrace(int32_t offset = 0, int32_t depth = StackTrace::kDefaultStackTraceDepth):
      frames_(nullptr),
      frames_len_(0){
      int32_t total_size = offset + depth;
      if(!(frames_ = (void**) malloc(sizeof(void*) * total_size))){
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

    CrashReport(const std::string& filename, const std::string& cause):
      filename_(filename),
      cause_(cause){}
   public:
    ~CrashReport() = default;

    std::string GetFilename() const{
      return filename_;
    }

    std::string GetCause() const{
      return cause_;
    }

    static bool PrintNewCrashReport(const std::string& cause, const google::LogSeverity& severity = google::ERROR);
    static bool PrintNewCrashReportAndExit(const std::string& cause,
      const google::LogSeverity& severity = google::ERROR,
      int code = EXIT_FAILURE);

    static inline bool
    PrintNewCrashReport(const std::stringstream& ss, const google::LogSeverity& severity = google::ERROR){
      return PrintNewCrashReport(ss.str(), severity);
    }

    static inline bool
    PrintNewCrashReportAndExit(const std::stringstream& ss,
      const google::LogSeverity& severity = google::ERROR,
      int code = EXIT_FAILURE){
      return PrintNewCrashReportAndExit(ss.str(), severity, code);
    }
  };

  class CrashReportPrinter : public Printer{
   private:
    std::string cause_;

    template<class T>
    bool PrintState(const std::string& name){
      LOG_AT_LEVEL(GetSeverity()) << name << ": " << T::GetState() << " [" << T::GetStatus() << "]";
      return true;
    }

    bool PrintBanner();
    bool PrintCrashInformation();
    bool PrintPeerInformation();
    bool PrintServerInformation();
    bool PrintRestServiceInformation();
    bool PrintHealthServiceInformation();
    bool PrintHostInformation();
    bool PrintSystemInformation();
   public:
    CrashReportPrinter(const std::string& cause,
      const google::LogSeverity& severity = google::INFO,
      const long& flags = Printer::kFlagNone):
      Printer(severity, flags),
      cause_(cause){}
    ~CrashReportPrinter() = default;

    std::string GetCause() const{
      return cause_;
    }

    bool Print(){
      if(!PrintBanner()){
        LOG(WARNING) << "couldn't print the banner.";
        return false;
      }

      if(!PrintSystemInformation()){
        LOG(WARNING) << "couldn't print the system information.";
        return false;
      }

#ifdef TOKEN_ENABLE_HEALTH_SERVICE
      if(!PrintHealthServiceInformation()){
        LOG(WARNING) << "couldn't print the health service information.";
        return false;
      }
#endif//TOKEN_ENABLE_HEALTH_SERVICE

#ifdef TOKEN_ENABLE_REST_SERVICE
      if(!PrintRestServiceInformation()){
        LOG(WARNING) << "couldn't print the rest service information.";
        return false;
      }
#endif//TOKEN_ENABLE_REST_SERVICE

#ifdef TOKEN_ENABLE_SERVER
      if(!PrintServerInformation()){
        LOG(WARNING) << "couldn't print the server information.";
        return false;
      }

      if(!PrintPeerInformation()){
        LOG(WARNING) << "couldn't print the peer information.";
        return false;
      }
#endif//TOKEN_ENABLE_SERVER

      if(!PrintCrashInformation()){
        LOG(WARNING) << "couldn't print the crash information.";
        return false;
      }
      return true;
    }
  };
}

#endif //TOKEN_CRASH_REPORT_H