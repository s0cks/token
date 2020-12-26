#include "crash_report_writer.h"
#include "server.h"

namespace Token{
    bool CrashReportWriter::WriteBanner(){
        // Print Debug Banner in Logs
        std::string header = "Token v" + Token::GetVersion();
        size_t total_size = 50;
        size_t middle = (total_size - header.size()) / 2;

        std::stringstream ss1;
        for(size_t idx = 0; idx < total_size; idx++) ss1 << "#";

        std::stringstream ss2;
        ss2 << "#";
        for(size_t idx = 0; idx < middle; idx++) ss2 << " ";
        ss2 << header;
        for(size_t idx = 0; idx < middle - 2; idx++) ss2 << " ";
        ss2 << "#";

        return WriteLine(ss1)
            && WriteLine(ss2)
            && WriteLine(ss1);
    }

    static inline std::string
    GetDebugStatus(){
#ifdef TOKEN_DEBUG
        return "Enabled";
#else
        return "Disabled";
#endif//TOKEN_DEBUG
    }

    bool CrashReportWriter::WriteSystemInformation(){
        return WriteLine("Timestamp: " + GetTimestampFormattedReadable(GetCurrentTimestamp()))
            && WriteLine("Version: " + GetVersion())
            && WriteLine("Debug Mode: " + GetDebugStatus())
            && WriteLine("Ledger Home: " + TOKEN_BLOCKCHAIN_HOME);
    }

    bool CrashReportWriter::WriteServerInformation(){
        PeerList peers;
        //TODO: write peer information
        return true;
    }

    bool CrashReportWriter::WriteStackTrace(){
        Indent();
        int32_t offset = 4;
        int32_t depth = 20;
        StackTrace trace(offset, depth);
        for(int32_t idx = offset; idx < trace.GetNumberOfFrames(); idx++){
            std::stringstream ss;
            ss << (idx + 1 - offset) << ": " << trace[idx];
            WriteLine(ss);
        }
        DeIndent();
        return true;
    }

    bool CrashReportWriter::WriteCrashReport(){
        LOG(INFO) << "generating crash report....";
        if(!WriteBanner()){
            LOG(WARNING) << "couldn't write crash report banner";
            return false;
        }

        if(!WriteSystemInformation() || !NewLine()){
            LOG(WARNING) << "couldn't write system information to crash report";
            return false;
        }

        if(!WriteServerInformation() || !NewLine()){
            LOG(WARNING) << "couldn't write server information to crash report";
            return false;
        }

        if(!WriteStackTrace() || !NewLine()){
            LOG(WARNING) << "couldn't write stack trace to crash report.";
            return false;
        }

        //TODO:
        // - write block chain information
        // - write thread information
        LOG(INFO) << "crash report generated: " << GetFilename();
        return true;
    }
}