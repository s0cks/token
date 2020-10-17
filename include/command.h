#ifndef TOKEN_COMMAND_H
#define TOKEN_COMMAND_H

#include <deque>
#include <string>
#include "Hash.h"

namespace Token{
    class Command{
    protected:
        std::string name_;
        std::deque<std::string> args_;

        Command(const std::string& name, std::deque<std::string>& args):
            name_(name),
            args_(args){}
        Command(std::deque<std::string>& args):
            name_(GetCommand(args)),
            args_(args){}

        static inline std::string
        GetCommand(std::deque<std::string>& args){
            std::string cmd = args.front();
            args.pop_front();
            return cmd;
        }
    public:
        virtual ~Command() = default;

        std::string GetName() const{
            return name_;
        }

        size_t GetArgumentCount() const{
            return args_.size();
        }

        bool HasNextArgument() const{
            return !args_.empty();
        }

        std::string GetNextArgument(){
            std::string arg = args_.front();
            args_.pop_front();
            return arg;
        }

        Hash GetNextArgumentHash(){
            return Hash::FromHexString(GetNextArgument());
        }

        uint32_t GetNextArgumentUnsignedInt(){
            return (uint32_t)atol(GetNextArgument().c_str());
        }

        friend std::ostream& operator<<(std::ostream& stream, const Command& cmd){
            std::vector<std::string> args(cmd.args_.begin(), cmd.args_.end());
            stream << cmd.GetName();
            stream << "(";
            for(size_t idx = 0; idx < args.size(); idx++){
                stream << args[idx];
                if(idx < (args.size() - 1)) stream << ",";
            }
            stream << ")";
            return stream;
        }
    };
}

#endif //TOKEN_COMMAND_H