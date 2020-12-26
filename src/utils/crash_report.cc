#include <glog/logging.h>
#include "utils/crash_report.h"
#include "crash_report_writer.h"
#include "crash_report_printer.h"

namespace Token{
    bool CrashReport::PrintNewCrashReport(const std::string& cause, const google::LogSeverity& severity){
        CrashReportPrinter printer(severity);
        return printer.Print();
    }

    bool CrashReport::PrintNewCrashReportAndExit(const std::string& cause, const google::LogSeverity& severity, int code){
        CrashReportPrinter printer(severity);
        return printer.Print();
    }

    bool CrashReport::WriteNewCrashReport(const std::string& cause){
        CrashReportWriter writer(GetNewCrashReportFilename());
        return writer.WriteCrashReport();
    }

    void CrashReport::WriteNewCrashReportAndExit(const std::string &msg){
        if(!WriteNewCrashReport(msg)) fprintf(stderr, "couldn't generate crash report!\n");
        exit(EXIT_FAILURE);
    }
}