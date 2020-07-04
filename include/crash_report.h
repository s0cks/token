#ifndef TOKEN_CRASH_REPORT_H
#define TOKEN_CRASH_REPORT_H

#include <fstream>
#include "common.h"

namespace Token{
    class CrashReport{
    private:
        std::fstream file_;
        std::string message_;

        inline std::fstream&
        GetStream(){
            return file_;
        }

        CrashReport(const std::string& filename, const std::string& msg):
            file_(filename, std::ios::out),
            message_(msg){}
    public:
        std::string GetMessage() const{
            return message_;
        }

        inline bool
        WriteLine(const std::string& line){
            GetStream() << line << std::endl;
            return true;
        }

        inline bool
        WriteLine(const std::stringstream& line){
            GetStream() << line.str() << std::endl;
            return true;
        }

        inline bool
        WriteNewline(){
            GetStream() << std::endl;
            return true;
        }

        friend CrashReport& operator<<(CrashReport& stream, const std::string& line){
            stream.GetStream() << line << std::endl;
            return stream;
        }

        friend CrashReport& operator<<(CrashReport& stream, const std::stringstream& line){
            stream.GetStream() << line.str() << std::endl;
            return stream;
        }

        friend CrashReport& operator<<(CrashReport& stream, const bool& value){
            stream.GetStream() << value;
            return stream;
        }

        static bool Generate(const std::string& msg);
        static int GenerateAndExit(const std::string& msg);

        static inline int
        GenerateAndExit(const std::stringstream& ss){
            return GenerateAndExit(ss.str());
        }
    };
}

#endif //TOKEN_CRASH_REPORT_H