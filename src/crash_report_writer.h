#ifndef TOKEN_CRASH_REPORT_WRITER_H
#define TOKEN_CRASH_REPORT_WRITER_H

#include "utils/file_writer.h"
#include "crash_report.h"

namespace Token{
    class CrashReportWriter : public TextFileWriter{
    private:
        bool WriteBanner();
        bool WriteSystemInformation();
        bool WriteServerInformation();
        bool WriteStackTrace();
    public:
        CrashReportWriter(const std::string& filename):
            TextFileWriter(filename){}
        ~CrashReportWriter() = default;

        bool WriteCrashReport();
    };
}

#endif //TOKEN_CRASH_REPORT_WRITER_H