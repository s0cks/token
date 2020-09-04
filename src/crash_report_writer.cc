#include "crash_report_writer.h"
#include "token.h"
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
    GetStatus(bool status){
        return status ?
            "Enabled" :
            "Disabled";
    }

    bool CrashReportWriter::WriteSystemInformation(){
        return WriteLine("Timestamp: " + GetTimestampFormattedReadable(GetCurrentTimestamp()))
            && WriteLine("Version: " + GetVersion())
            && WriteLine("Debug Mode: " + GetStatus(TOKEN_DEBUG))
            && WriteLine("Ledger Home: " + TOKEN_BLOCKCHAIN_HOME);
    }

    bool CrashReportWriter::WriteServerInformation(){
        std::vector<PeerInfo> peers;
        if(!Server::GetPeers(peers)){
            LOG(WARNING) << "couldn't get peer info from server";
            return false;
        }

        {
            std::stringstream ss;
            ss << "Peers (" << peers.size() << ") ";
            if(!WriteLine(ss)) return false;
        }
        for(auto& it : peers){
            std::stringstream ss;
            ss << "  - " << it << std::endl;
            ss << "\t<HEAD> := " << it.GetHead()->GetHeader() << std::endl;
            if(!Write(ss)) return false;
        }
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

        //TODO:
        // - write block chain information
        // - write thread information
        // - write stack trace information

        LOG(INFO) << "crash report generated: " << GetFilename();
        return true;
    }
}