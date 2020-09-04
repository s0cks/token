#include <glog/logging.h>
#include "crash_report.h"
#include "crash_report_writer.h"

namespace Token{
    bool CrashReport::WriteNewCrashReport(const std::string& cause){
        CrashReportWriter writer(GetNewCrashReportFilename());
        return writer.WriteCrashReport();
    }

    void CrashReport::WriteNewCrashReportAndExit(const std::string &msg){
        if(!WriteNewCrashReport(msg)) fprintf(stderr, "couldn't generate crash report!\n");
        exit(EXIT_FAILURE);
    }
}