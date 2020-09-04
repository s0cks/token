#ifndef TOKEN_CRASH_REPORT_H
#define TOKEN_CRASH_REPORT_H

#include <fstream>
#include "common.h"

namespace Token{
    class CrashReport{
    private:
        std::string filename_;
        std::string cause_;

        CrashReport(const std::string& filename, const std::string& cause):
            filename_(filename),
            cause_(cause){}

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