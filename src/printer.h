#ifndef TOKEN_PRINTER_H
#define TOKEN_PRINTER_H

namespace Token{
    class Printer{
    public:
        static const int kFlagNone = 0;
        static const int kFlagDetailed = 1 << 1;
    protected:
        Printer* parent_;
        google::LogSeverity severity_;
        long flags_;
        int32_t indent_;

        Printer(Printer* parent,
                const google::LogSeverity& severity,
                const long& flags):
            parent_(parent),
            severity_(severity),
            flags_(flags){}
        Printer(const google::LogSeverity& severity, const long& flags):
            parent_(nullptr),
            severity_(severity),
            flags_(flags){}

        long GetFlags() const{
            return flags_;
        }
    public:
        virtual ~Printer() = default;

        Printer* GetParent() const{
            return parent_;
        }

        bool IsDetailed() const{
            return (flags_ & kFlagDetailed) == kFlagDetailed;
        }

        google::LogSeverity GetSeverity() const{
            return severity_;
        }
    };

    class ToStringPrinter : public Printer{
    public:
        ToStringPrinter(Printer* parent, const google::LogSeverity& severity, const long& flags):
            Printer(parent, severity, flags){}
        ToStringPrinter(const google::LogSeverity severity=google::INFO, const long& flags=Printer::kFlagNone):
            Printer(nullptr, severity, flags){}
        ToStringPrinter(const ToStringPrinter& other):
            Printer(other.GetParent(), other.GetSeverity(), other.GetFlags()){}
        ~ToStringPrinter() = default;

        bool Print(Object* obj) const{
            LOG_AT_LEVEL(GetSeverity()) << obj->ToString();
            return true;
        }
    };

    class HashPrinter : public Printer{
    public:
        HashPrinter(Printer* parent, const google::LogSeverity& severity, const long& flags):
            Printer(parent, severity, flags){}
        HashPrinter(const google::LogSeverity& severity=google::INFO, const long& flags=Printer::kFlagNone):
            Printer(nullptr, severity, flags){}
        ~HashPrinter() = default;

        bool Print(BinaryObject* obj) const{
            LOG_AT_LEVEL(GetSeverity()) << obj->GetHash();
            return true;
        }
    };
}

#endif //TOKEN_PRINTER_H