#ifndef TOKEN_CRASH_REPORT_H
#define TOKEN_CRASH_REPORT_H

#include <cstdio>
#include <cstdarg>
#include "common.h"

namespace Token{
  class CrashReport{
   private:
    std::string filename_;
    FILE* file_;

    Timestamp timestamp_;
    std::string cause_;

    FILE* GetFile() const{
      return file_;
    }

    inline void
    Print(const char* fmt, ...) const{
      if(!file_)
        return;

      va_list args;
      va_start(args, fmt);
      vfprintf(file_, fmt, args);
      va_end(args);
    }
   public:
    CrashReport(FILE* file, const std::string& cause):
      filename_(),
      file_(file),
      timestamp_(Clock::now()),
      cause_(cause){}
    CrashReport(const std::string& filename, const std::string& cause):
      filename_(filename),
      file_(NULL),
      timestamp_(Clock::now()),
      cause_(cause){
      if((file_ = fopen(filename.c_str(), "w")) == NULL)
        LOG(WARNING) << "couldn't create text file " << filename << ": " << strerror(errno);
    }
    ~CrashReport(){
      if(file_ && (fclose(file_) != 0)){
        LOG(WARNING) << "failed to close crash report file " << filename_ << ": " << strerror(errno);
      }
    }

    std::string GetFilename() const{
      return filename_;
    }

    Timestamp GetTimestamp() const{
      return timestamp_;
    }

    std::string GetCause() const{
      return cause_;
    }

    bool Print() const;

    static inline bool
    PrintNewCrashReport(const std::string& cause, const std::string& filename){
      CrashReport report(filename, cause);
      return report.Print();
    }

    static inline bool
    PrintNewCrashReport(const std::string& cause, FILE* file=stderr){
      CrashReport report(file, cause);
      return report.Print();
    }

    static inline bool
    PrintNewCrashReport(const std::stringstream& cause, const std::string& filename){
      CrashReport report(filename, cause.str());
      return report.Print();
    }

    static inline bool
    PrintNewCrashReport(const std::stringstream& cause, FILE* file=stderr){
      CrashReport report(file, cause.str());
      return report.Print();
    }
  };
}

#endif//TOKEN_CRASH_REPORT_H