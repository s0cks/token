#ifndef TOKEN_PRINTER_H
#define TOKEN_PRINTER_H

namespace Token{
    class Printer{
    protected:
        google::LogSeverity severity_;

        Printer(const google::LogSeverity& severity):
            severity_(severity){}
    public:
        virtual ~Printer() = default;

        google::LogSeverity GetSeverity() const{
            return severity_;
        }
    };

    class ToStringPrinter : public Printer{
    public:
        ToStringPrinter(const google::LogSeverity& severity): Printer(severity){}
        ~ToStringPrinter() = default;

        bool Print(Object* obj) const{
            LOG_AT_LEVEL(GetSeverity()) << obj->ToString();
            return true;
        }
    };

    class HashPrinter : public Printer{
    public:
        HashPrinter(const google::LogSeverity& severity): Printer(severity){}
        ~HashPrinter() = default;

        bool Print(BinaryObject* obj) const{
            LOG_AT_LEVEL(GetSeverity()) << obj->GetHash();
            return true;
        }
    };
}

#endif //TOKEN_PRINTER_H