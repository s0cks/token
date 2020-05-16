#ifndef TOKEN_CRASH_REPORT_H
#define TOKEN_CRASH_REPORT_H

#include <string>
#include <fstream>
#include <time.h>
#include "common.h"
#include "block_chain.h"

namespace Token{
    class Printer;

    class CrashReport{
    public:
        static const size_t kBannerSize = 32;

        enum Flags{
            kNone = 1 << 0,
            //TODO: define
        };
    private:
        bool Accept(Printer* printer);
    protected:
        long flags_;
        std::string cause_;

        long GetFlags() const{
            return flags_;
        }

        virtual bool Print(const std::string& value) = 0;

        inline bool Print(const std::stringstream& stream){
            return Print(stream.str());
        }

        CrashReport(const std::string& cause, long flags):
            cause_(cause),
            flags_(flags){}

        friend class Printer;
    public:
        virtual ~CrashReport() = default;

        std::string GetCause() const{
            return cause_;
        }
    };

    class CrashReportFile : public CrashReport{
    private:
        /*
         * static std::string CreateNewFilename(){
            std::stringstream stream;
            stream << FLAGS_path << "/crash-";
            time_t raw_time;
            time(&raw_time);
            struct tm* timeinfo = localtime(&raw_time);
            char buffer[256];
            strftime(buffer, sizeof(buffer), "%Y%m%d-%H%M%S", timeinfo);
            stream << buffer << ".log";
            return stream.str();
        }
         */

        std::fstream stream_;

        inline std::fstream&
        GetStream(){
            return stream_;
        }
    public:
        CrashReportFile(const std::string& filename, const std::string& cause, long flags):
            stream_(filename, std::ios::out),
            CrashReport(cause, flags){}
        ~CrashReportFile(){}

        bool Print(const std::string& value){
            GetStream() << value;
            return true;
        }
    };

    class CrashReportLog : public CrashReport{
    private:
        google::LogSeverity severity_;

        inline google::LogSeverity
        GetSeverity(){
            return severity_;
        }
    public:
        CrashReportLog(google::LogSeverity severity, const std::string& cause="", long flags=CrashReport::Flags::kNone):
            severity_(severity),
            CrashReport(cause, flags){}
        ~CrashReportLog(){}

        bool Print(const std::string& value){
            LOG_AT_LEVEL(GetSeverity()) << value;
            return true;
        }
    };

    class Printer{
    public:
        enum Flags{
            kNone = 1 << 0,
            kDetailed = 1 << 1,
            kTabIndent = 1 << 2,
        };
    protected:
        CrashReport* report_;
        int indent_;
        long flags_;

        CrashReport* GetCrashReport() const{
            return report_;
        }

        int GetIndent() const{
            return indent_;
        }

        void Indent(){
            indent_++;
        }

        void DeIndent(){
            indent_--;
        }

        bool Print(const std::string& value){
            std::stringstream line;
            for(int indent = 0; indent < GetIndent(); indent++) line << (IsTabIndent() ? '\t' : ' ');
            line << value;
            return GetCrashReport()->Print(line);
        }

        inline bool Print(const std::stringstream& value){
            return Print(value.str());
        }

        inline std::string FormatTimestamp(uint32_t timestamp){
            return "";
        }

        Printer(CrashReport* report, int indent, long flags):
            report_(report),
            indent_(indent),
            flags_(flags){}
        Printer(Printer* parent):
            report_(parent->report_),
            indent_(parent->indent_ + 1),
            flags_(parent->flags_){}
    public:
        virtual ~Printer() = default;
        virtual bool Print() = 0;

        bool IsDetailed() const{
            return (flags_ & kDetailed) == kDetailed;
        }

        bool IsTabIndent() const{
            return (flags_ & kTabIndent) == kTabIndent;
        }
    };

#define DECLARE_PRINTER(Name) \
    static bool PrintAs(google::LogSeverity severity, int indent=0, long flags=Printer::Flags::kNone){ \
        CrashReportLog report(severity); \
        Name##Printer instance(&report, indent, flags); \
        return instance.Print(); \
    } \
    static bool PrintAsInfo(int indent=0, long flags=Printer::Flags::kNone){ \
        return PrintAs(google::GLOG_INFO, indent, flags); \
    } \
    static bool PrintAsWarning(int indent=0, long flags=Printer::Flags::kNone){ \
        return PrintAs(google::GLOG_WARNING, indent, flags); \
    } \
    static bool PrintAsError(int indent=0, long flags=Printer::Flags::kNone){ \
        return PrintAs(google::GLOG_ERROR, indent, flags); \
    } \
    static bool PrintAsFatal(int indent=0, long flags=Printer::Flags::kNone){ \
        return PrintAs(google::GLOG_FATAL, indent, flags); \
    }

    class BlockChainPrinter : public BlockChainVisitor, public Printer{
    public:
        BlockChainPrinter(CrashReport* report, int indent=0, long flags=Printer::Flags::kNone):
            BlockChainVisitor(),
            Printer(report, indent, flags){}
        BlockChainPrinter(Printer* parent):
            BlockChainVisitor(),
            Printer(parent){}
        ~BlockChainPrinter(){}

        bool Visit(const Block& block){
            std::stringstream line;
            line << "#" << block.GetHeight() << ": " << block.GetHash();
            if(IsDetailed()){
                Indent();
                std::stringstream timestamp;
                timestamp << "Timestamp: " << FormatTimestamp(block.GetTimestamp());
                Printer::Print(timestamp);

                std::stringstream prev_hash;
                prev_hash << "Previous Hash: " << block.GetPreviousHash();
                Printer::Print(prev_hash);

                std::stringstream merkle;
                merkle << "Merkle Root: " << block.GetMerkleRoot();
                Printer::Print(merkle);

                std::stringstream txs;
                txs << "Number of Transactions: " << block.GetNumberOfTransactions();
                Printer::Print(txs);
                DeIndent();
            }
            return true;
        }

        bool Print(){
            return BlockChain::Accept(this);
        }

        DECLARE_PRINTER(BlockChain);
    };

    class UnclaimedTransactionPoolPrinter : public UnclaimedTransactionPoolVisitor, public Printer{
    private:
        uint32_t index_;
    public:
        UnclaimedTransactionPoolPrinter(CrashReport* report, int indent=0, long flags=Printer::Flags::kNone):
            index_(0),
            UnclaimedTransactionPoolVisitor(),
            Printer(report, indent, flags){}
        UnclaimedTransactionPoolPrinter(Printer* parent):
            index_(0),
            UnclaimedTransactionPoolVisitor(),
            Printer(parent){}
        ~UnclaimedTransactionPoolPrinter(){}

        bool Visit(const UnclaimedTransaction& utxo){
            std::stringstream line;
            line << "#" << index_++ << " ";
            if(IsDetailed()){
                line << utxo.GetTransaction() << "[" << utxo.GetIndex() << "]";
            } else{
                line << utxo.GetHash();
            }
            return Printer::Print(line);
        }

        bool Print(){
            return UnclaimedTransactionPool::Accept(this);
        }

        DECLARE_PRINTER(UnclaimedTransactionPool);
    };
}

#endif //TOKEN_CRASH_REPORT_H