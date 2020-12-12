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

        Printer(Printer* parent):
            parent_(parent),
            severity_(parent->GetSeverity()),
            flags_(parent->flags_){}
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
        ToStringPrinter(const google::LogSeverity severity=google::INFO, const long& flags=Printer::kFlagNone):
            Printer(severity, flags){}
        ToStringPrinter(Printer* parent):
            Printer(parent){}
        ~ToStringPrinter() = default;

        bool Print(Object* obj) const{
            LOG_AT_LEVEL(GetSeverity()) << obj->ToString();
            return true;
        }
    };

    class HashPrinter : public Printer{
    public:
        HashPrinter(const google::LogSeverity& severity=google::INFO, const long& flags=Printer::kFlagNone):
            Printer(severity, flags){}
        HashPrinter(Printer* parent):
            Printer(parent){}
        ~HashPrinter() = default;

        bool Print(BinaryObject* obj) const{
            LOG_AT_LEVEL(GetSeverity()) << obj->GetHash();
            return true;
        }
    };
}

#endif //TOKEN_PRINTER_H