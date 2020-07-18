#include <glog/logging.h>
#include <time.h>
#include <execinfo.h>


#include "allocator.h"

#include "token.h"
#include "crash_report.h"
#include "block_chain.h"
#include "node/node.h"

namespace Token{
    class CrashReportSection{
    public:
        static const size_t kDefaultHeaderSize = 100;
    protected:
        CrashReport* report_;
        int indent_;

        CrashReport* GetCrashReport() const{
            return report_;
        }

        inline void
        Indent(){
            indent_++;
        }

        inline void
        DeIndent(){
            indent_--;
        }

        inline void
        WriteLine(const std::string& line) const{
            std::stringstream ss;
            for(int idx = 0; idx < indent_; idx++) ss << " ";
            ss << line;
            GetCrashReport()->WriteLine(ss);
        }

        inline void
        WriteLine(const std::stringstream& line) const{
            WriteLine(line.str());
        }

        inline void
        WriteNewline(){
            GetCrashReport()->WriteNewline();
        }

        inline std::string
        GetTimestampFormatted(uint32_t current_time){
            using namespace std::chrono;
            std::time_t timeval = current_time;
            std::tm* tm = std::gmtime(&timeval);
            std::stringstream ss;
            ss << std::put_time(tm, "%m/%d/%Y %H:%M:%S");
            return ss.str();
        }

        inline std::string
        GetCurrentTimeFormatted(){
            return GetTimestampFormatted(GetCurrentTime());
        }

        inline void
        WriteHeader(const std::string& header, size_t size=kDefaultHeaderSize){
            size_t middle = (size - header.size()) / 2;

            std::stringstream ls;
            for(size_t idx = 0; idx < size; idx++) ls << "#";

            std::stringstream info;
            info << "#";
            for(size_t idx = 0; idx < middle - 1; idx++) info << " ";
            info << header;
            for(size_t idx = 0; idx < middle; idx++) info << " ";
            info << "#";

            GetCrashReport()->WriteLine(ls);
            GetCrashReport()->WriteLine(info);
            GetCrashReport()->WriteLine(ls);
        }

        CrashReportSection(CrashReport* report):
            indent_(0),
            report_(report){}
    public:
        virtual ~CrashReportSection() = default;

        virtual bool WriteSection() = 0;
    };

    class SystemInformationSection : public CrashReportSection{
    private:
        static inline std::string
        GetDebugStatus(){
#ifdef TOKEN_DEBUG
            return "Enabled";
#else
            return "Disabled";
#endif//TOKEN_DEBUG
        }

        static inline std::string
        GetPeerCount(){
            std::stringstream ss;
            ss << Node::GetNumberOfPeers();
            return ss.str();
        }

        static inline std::string
        GetBlockMinerThreadStatus(){
            switch(BlockMiner::GetState()){
                case BlockMiner::kStarting: return "Starting";
                case BlockMiner::kRunning: return "Running";
                case BlockMiner::kPaused: return "Paused";
                case BlockMiner::kStopped: return "Stopped";
                default: return "Unknown";
            }
        }

        static inline std::string
        GetNodeThreadStatus(){
            switch(Node::GetState()){
                case Node::kStarting: return "Starting";
                case Node::kSynchronizing: return "Synchronizing";
                case Node::kRunning: return "Running";
                case Node::kStopping: return "Stopping";
                case Node::kStopped: return "Stopped";
                default: return "Unknown";
            }
        }

        static inline std::string
        GetPeerThreadStatus(const PeerInfo& info){
            std::stringstream ss;
            ss << info.GetID() << ": ";
            switch(info.GetState()){
                case PeerInfo::kConnecting:
                    ss << "Connecting";
                    break;
                case PeerInfo::kConnected:
                    ss << "Connected";
                    break;
                case PeerInfo::kDisconnected:
                    ss << "Disconnected";
                    break;
                default:
                    ss << "Unknown";
                    break;
            }
        }
    public:
        SystemInformationSection(CrashReport* report): CrashReportSection(report){}
        ~SystemInformationSection(){}

        bool WriteSection(){
            WriteHeader("Token v" + Token::GetVersion() + " Crash Report");
            WriteLine("Debug Mode: " + GetDebugStatus());
            WriteLine("Version: " + Token::GetVersion());
            WriteLine("Current Time: " + GetCurrentTimeFormatted());
            WriteLine("Number of Peers: " + GetPeerCount());

            {
                // Write Thread Status
                WriteLine("Threads:");
                Indent();
                WriteLine("Block Miner: " + GetBlockMinerThreadStatus());
                WriteLine("Node: " + GetNodeThreadStatus());
                WriteLine("Peers:");

                {
                    // Write Peer Statuses
                    Indent();
                    std::vector<PeerInfo> peers;
                    Node::GetPeers(peers);
                    for(auto& it : peers){
                        WriteLine(GetPeerThreadStatus(it));
                    }
                    DeIndent();
                }
                DeIndent();
            }

            WriteLine("Cause: " + GetCrashReport()->GetMessage());
            WriteNewline();
            return true;
        }
    };

    class MemoryInformationSection : public CrashReportSection{
    public:
        MemoryInformationSection(CrashReport* report): CrashReportSection(report){}
        ~MemoryInformationSection(){}

#if defined(TOKEN_USE_KOA)
        inline std::string
        GetHeapSize() const{
            std::stringstream ss;
            ss << Allocator::GetEdenHeap()->GetTotalSize();
            return ss.str();
        }

        inline std::string
        GetSemispaceSize() const{
            std::stringstream ss;
            ss << Allocator::GetEdenHeap()->GetSemispaceSize();
            return ss.str();
        }
#endif //TOKEN_USE_KOA
        inline std::string
        GetTotalMemoryUsed() const{
            std::stringstream ss;
            return ss.str();
        }

        inline std::string
        GetTotalMemoryFree() const{
            std::stringstream ss;
            return ss.str();
        }

        inline std::string
        GetNumberOfAllocatedObjects() const{
            std::stringstream ss;
            return ss.str();
        }

        bool WriteSection(){
            WriteLine("Memory Information:");
            Indent();
#if defined(TOKEN_USE_KOA)
            WriteLine("Garbage Collector: Mark/Copy");
#else
            WriteLine("Garbage Collector: Mark/Sweep");
#endif //TOKEN_USE_KOA
            WriteLine("Number of Allocated Objects: " + GetNumberOfAllocatedObjects());
            WriteLine("Total Memory Used (Bytes): " + GetTotalMemoryUsed());
            WriteLine("Total Memory Free (Bytes): " + GetTotalMemoryFree());
#if defined(TOKEN_USE_KOA)
            WriteLine("Heap Size (Bytes): " + GetHeapSize());
            WriteLine("Semispace Size (Bytes): " + GetSemispaceSize());
#endif//TOKEN_USE_KOA
            DeIndent();
            WriteNewline();
            return true;
        }
    };

    class StackTraceSection : public CrashReportSection{
    public:
        static const size_t kBacktraceDepth = 10;
        static const size_t kBacktraceOffset = 4;
        static const size_t kBacktraceSize = kBacktraceOffset+kBacktraceDepth;

        StackTraceSection(CrashReport* report): CrashReportSection(report){}
        ~StackTraceSection(){}

        //TODO:
        // - better stack trace
        // - allow different threads
        bool WriteSection(){
            WriteLine("Stack Trace:");
            Indent();

            void* bt_ptr[kBacktraceSize];
            size_t bt_size = backtrace(bt_ptr, kBacktraceSize);
            char** bt = backtrace_symbols(bt_ptr, kBacktraceSize);

            for(size_t idx = kBacktraceOffset; idx < bt_size; idx++){
                std::stringstream ss;
                ss << (idx + 1 - kBacktraceOffset) << ": " << bt[idx];
                WriteLine(ss);
            }

            WriteNewline();
            return true;
        }
    };

    class BlockChainSection : public CrashReportSection, public BlockChainVisitor{
    public:
        BlockChainSection(CrashReport* report): CrashReportSection(report){}
        ~BlockChainSection(){}

        bool Visit(const BlockHeader& block) const{
            std::stringstream ss;
            ss << "- " << block;
            WriteLine(ss);
            return true;
        }

        bool WriteSection(){
            if(BlockChain::IsInitialized()){
                WriteLine("Block Chain:");
                Indent();
                {
                    BlockHeader head = BlockChain::GetHead();
                    WriteLine("Head:");
                    Indent();
                    {
                        std::stringstream height;
                        height << "Height: " << head.GetHeight();
                        WriteLine(height); //TODO: fixme, weird design
                        WriteLine("Hash: " + HexString(head.GetHash()));
                        WriteLine("Previous Hash: " + HexString(head.GetPreviousHash()));
                        WriteLine("Merkle Root: " + HexString(head.GetMerkleRoot()));
                        WriteLine("Timestamp: " + GetTimestampFormatted(head.GetTimestamp()));
                    }
                    DeIndent();
                    WriteNewline();
                }
                DeIndent();

                Indent();
                {
                    WriteLine("Blocks:");
                    Indent();
                    BlockChain::Accept(this);
                    WriteNewline();
                }
            } else{
                std::stringstream ss;
                ss << "BlockChain: ";
                switch(BlockChain::GetState()){
                    case BlockChain::kInitialized:
                        ss << "Initialized";
                        break;
                    case BlockChain::kInitializing:
                        ss << "Initializing";
                        break;
                    case BlockChain::kUninitialized:
                        ss << "Uninitialized";
                        break;
                    default:
                        ss << "Unknown";
                        break;
                }
                WriteLine(ss);
            }

            return true;
        }
    };

    static inline std::string
    GenerateCrashReportFilename(){
        std::stringstream filename;
        filename << FLAGS_path << "/crash-";
        time_t current_time;
        time(&current_time);
        struct tm* timeinfo = localtime(&current_time);
        char buff[256];
        strftime(buff, sizeof(buff), "%Y%m%d-%H%M%S", timeinfo);
        filename << buff << ".log";
        return filename.str();
    }

    bool CrashReport::Generate(const std::string &msg){
        std::string filename = GenerateCrashReportFilename();
        CrashReport report(filename, msg);

        SystemInformationSection sys_info(&report);
        if(!sys_info.WriteSection()){
            fprintf(stderr, "couldn't write System Information to crash report: %s\n", filename.c_str());
            return false;
        }

        StackTraceSection stack_info(&report);
        if(!stack_info.WriteSection()){ //TODO: make stateful
            fprintf(stderr, "couldn't write stack trace to crash report: %s\n", filename.c_str());
            return false;
        }

        MemoryInformationSection mem_info(&report);
        if(!mem_info.WriteSection()){ //TODO: make stateful
            fprintf(stderr, "couldn't write memory information to crash report: %s\n", filename.c_str());
            return false;
        }

        BlockChainSection info(&report);
        if(!info.WriteSection()){
            fprintf(stderr, "couldn't write block chain information to crash report: %s\n", filename.c_str());
            return false;
        }

        LOG(ERROR) << "A crash report has been generated in: " << filename;
        return true;
    }

    int CrashReport::GenerateAndExit(const std::string &msg){
        if(!Generate(msg)) fprintf(stderr, "couldn't generate crash report!\n");
        exit(EXIT_FAILURE);
    }
}