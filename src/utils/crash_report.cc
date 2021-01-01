#include <glog/logging.h>
#include "utils/crash_report.h"
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
}