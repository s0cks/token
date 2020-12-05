#ifndef TOKEN_PRINTER_H
#define TOKEN_PRINTER_H

namespace Token{
    class Printer{
    protected:
        google::LogSeverity severity_;

        Printer(const google::LogSeverity& severity):
            severity_(severity){}

        std::ostream& Stream(){
            return LOG_AT_LEVEL(GetSeverity());
        }
    public:
        virtual ~Printer() = default;

        google::LogSeverity GetSeverity() const{
            return severity_;
        }
    };
}

#endif //TOKEN_PRINTER_H