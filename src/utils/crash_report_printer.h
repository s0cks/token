#ifndef TOKEN_CRASH_REPORT_PRINTER_H
#define TOKEN_CRASH_REPORT_PRINTER_H

#include "server.h"
#include "version.h"
#include "utils/printer.h"
#include "utils/crash_report.h"
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
#ifdef OS_IS_LINUX
            LOG_AT_LEVEL(GetSeverity()) << "Operating System: Linux";
#elif OS_IS_WINDOWS
            LOG_AT_LEVEL(GetSeverity()) << "Operating System: Windows";
#elif OS_IS_OSX
            LOG_AT_LEVEL(GetSeverity()) << "Operating System: OSX";
#endif //Operating System
#ifdef ARCHITECTURE_IS_X64
            LOG_AT_LEVEL(GetSeverity()) << "Architecture: x64";
#elif ARCHITECTURE_IS_X32
            LOG_AT_LEVEL(GetSeverity()) << "Architecture: x32";
#endif //Architecture

            if(BlockChain::IsInitialized() && BlockChain::IsOk()){
                LOG_AT_LEVEL(GetSeverity()) << "Block Chain (" << BlockChain::GetStatus() << "):";
                LOG_AT_LEVEL(GetSeverity()) << "\tGenesis: " << BlockChain::GetReference(BLOCKCHAIN_REFERENCE_GENESIS);
                LOG_AT_LEVEL(GetSeverity()) << "\tHead: " << BlockChain::GetReference(BLOCKCHAIN_REFERENCE_HEAD);
            } else{
                LOG_AT_LEVEL(GetSeverity()) << "Block Chain (" << BlockChain::GetStatus() << "): " << BlockChain::GetState();
            }

            if(BlockPool::IsInitialized() && BlockPool::IsOk()){
                LOG_AT_LEVEL(GetSeverity()) << "Block Pool (" << BlockPool::GetStatus() << "):";
                LOG_AT_LEVEL(GetSeverity()) << "\tTotal Number of Blocks: " << BlockPool::GetNumberOfBlocksInPool();
            } else{
                LOG_AT_LEVEL(GetSeverity()) << "Block Pool (" << BlockPool::GetStatus() << "): " << BlockPool::GetState();
            }

            if(TransactionPool::IsInitialized() && TransactionPool::IsOk()){
                LOG_AT_LEVEL(GetSeverity()) << "Transaction Pool (" << TransactionPool::GetStatus() << "):";
                LOG_AT_LEVEL(GetSeverity()) << "\tTotal Number of Transactions: " << TransactionPool::GetSize();
            } else{
                LOG_AT_LEVEL(GetSeverity()) << "Transaction Pool (" << TransactionPool::GetStatus() << "): " << TransactionPool::GetState();
            }

            if(UnclaimedTransactionPool::IsInitialized() && UnclaimedTransactionPool::IsOk()){
                LOG_AT_LEVEL(GetSeverity()) << "Unclaimed Transaction Pool (" << UnclaimedTransactionPool::GetStatus() << "):";
                LOG_AT_LEVEL(GetSeverity()) << "\tTotal Number of Unclaimed Transactions: " << UnclaimedTransactionPool::GetNumberOfUnclaimedTransactions();
            } else{
                LOG_AT_LEVEL(GetSeverity()) << "Unclaimed Transaction Pool (" << UnclaimedTransactionPool::GetStatus() << "): " << UnclaimedTransactionPool::GetState();
            }

            if(Server::IsRunning() && Server::IsOk()){
                LOG_AT_LEVEL(GetSeverity()) << "Server (" << Server::GetStatus() << "): ";

                std::set<UUID> peers;
                if(PeerSessionManager::GetConnectedPeers(peers)){
                    LOG_AT_LEVEL(GetSeverity()) << "\tPeers:";
                    for(auto& it : peers){
                        std::shared_ptr<PeerSession> session = PeerSessionManager::GetSession(it);
                        UUID id = session->GetID();
                        NodeAddress addr = session->GetAddress();
                        LOG_AT_LEVEL(GetSeverity()) << "\t - " << id << " (" << addr << "): " << session->GetState() << " [" << session->GetStatus() << "]";
                    }
                }
            } else{
                LOG_AT_LEVEL(GetSeverity()) << "Server (" << Server::GetStatus() << "): " << Server::GetState();
            }
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
        CrashReportPrinter(const google::LogSeverity& severity=google::INFO, const long& flags=Printer::kFlagNone):
            Printer(severity, flags){}
        CrashReportPrinter(Printer* printer):
            Printer(printer){}
        ~CrashReportPrinter() = default;

        bool Print(){
            return PrintBanner()
                && PrintSystemInformation()
                && PrintStackTrace();
        }
    };
}

#endif //TOKEN_CRASH_REPORT_PRINTER_H