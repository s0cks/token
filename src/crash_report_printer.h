#ifndef TOKEN_CRASH_REPORT_PRINTER_H
#define TOKEN_CRASH_REPORT_PRINTER_H

#include "version.h"
#include "printer.h"
#include "crash_report.h"
#include "peer/peer_session_manager.h"

namespace Token{
    class CrashReportPrinter : public Printer{
    private:
        bool PrintBanner(){
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

            LOG_AT_LEVEL(GetSeverity()) << ss1.str();
            LOG_AT_LEVEL(GetSeverity()) << ss2.str();
            LOG_AT_LEVEL(GetSeverity()) << ss1.str();
            return true;
        }

        bool PrintSystemInformation(){
            LOG_AT_LEVEL(GetSeverity()) << "Timestamp: " << GetTimestampFormattedReadable(GetCurrentTimestamp());
            LOG_AT_LEVEL(GetSeverity()) << "Version: " << Version();
            LOG_AT_LEVEL(GetSeverity()) << "Debug Mode: " << (TOKEN_DEBUG ? "Enabled" : "Disabled");
            LOG_AT_LEVEL(GetSeverity()) << "Ledger Home: " << TOKEN_BLOCKCHAIN_HOME;

            LOG_AT_LEVEL(GetSeverity()) << "Block Chain:";
            LOG_AT_LEVEL(GetSeverity()) << "\tGenesis: " << BlockChain::GetReference(BLOCKCHAIN_REFERENCE_GENESIS);
            LOG_AT_LEVEL(GetSeverity()) << "\tHead: " << BlockChain::GetReference(BLOCKCHAIN_REFERENCE_HEAD);
            return true;
        }

        bool PrintServerInformation(){
            //TODO:
            // - print remaining session types?
            std::vector<std::string> statuses;
            if(!PeerSessionManager::GetStatus(statuses)){
                LOG_AT_LEVEL(GetSeverity()) << "Peers: Unavailable";
                return true;
            }
            LOG_AT_LEVEL(GetSeverity()) << "Peers:";
            for(auto& it : statuses)
                LOG_AT_LEVEL(GetSeverity()) << "\t" << it;
            return true;
        }

        bool PrintStackTrace(){
            LOG_AT_LEVEL(GetSeverity()) << "Stack Trace:";
            int32_t offset = 4;
            int32_t depth = 20;
            StackTrace trace(offset, depth);
            for(int32_t idx = offset; idx < trace.GetNumberOfFrames(); idx++)
                LOG_AT_LEVEL(GetSeverity()) << "\t" << (idx + 1 - offset) << ": " << trace[idx];
            return true;
        }
    public:
        CrashReportPrinter(Printer* printer, const google::LogSeverity& severity, const long& flags):
            Printer(printer, severity, flags){}
        CrashReportPrinter(const google::LogSeverity& severity=google::INFO, const long& flags=Printer::kFlagNone):
            Printer(nullptr, severity, flags){}
        ~CrashReportPrinter() = default;

        bool Print(){
            if(!PrintBanner()){
                LOG(ERROR) << "couldn't print crash report banner.";
                return false;
            }

            if(!PrintSystemInformation()){
                LOG(ERROR) << "couldn't print crash report system information.";
                return false;
            }

            if(!PrintServerInformation()){
                LOG(ERROR) << "couldn't print crash report server information.";
                return false;
            }

            if(!PrintStackTrace()){
                LOG(ERROR) << "couldn't print crash report stack trace information.";
                return false;
            }
            return true;
        }
    };
}

#endif //TOKEN_CRASH_REPORT_PRINTER_H