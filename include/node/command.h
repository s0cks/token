#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wextra-tokens"
#ifndef TOKEN_COMMAND_H
#define TOKEN_COMMAND_H

#include <deque>
#include "common.h"

namespace Token{
#define FOR_EACH_COMMAND(V) \
    V(Status, ".status", 0) \
    V(Disconnect, ".disconnect", 0) \
    V(Test, ".test", 0)
    //V(Exit, ".exit", 0)

#define FORWARD_DECLARE_COMMAND(Name, Text, Parameters) class Name##Command;
FOR_EACH_COMMAND(FORWARD_DECLARE_COMMAND);
#undef FORWARD_DECLARE_COMMAND

    class Command{
    protected:
        std::deque<std::string> args_;

        Command(const std::deque<std::string>& args):
            args_(args){}
    public:
        virtual ~Command() = default;

        virtual const char* GetName() = 0;
#define DECLARE_TYPECHECK(Name, Text, Parameters) \
    bool Is##Name##Command(){ return As##Name##Command() != nullptr; } \
    virtual Name##Command* As##Name##Command(){ return nullptr; }

    FOR_EACH_COMMAND(DECLARE_TYPECHECK);

        static Command* ParseCommand(const std::string& line);
    };

#define DECLARE_COMMAND(Name) \
    public: \
        virtual Name##Command* As##Name##Command(){ return this; } \
        const char* GetName(){ return #Name; } \

    class StatusCommand : public Command{
    public:
        StatusCommand(const std::deque<std::string>& args): Command(args){}
        ~StatusCommand() = default;

        DECLARE_COMMAND(Status);
    };

    class DisconnectCommand : public Command{
    public:
        DisconnectCommand(const std::deque<std::string>& args): Command(args){}
        ~DisconnectCommand() = default;

        DECLARE_COMMAND(Disconnect);
    };

    class TestCommand : public Command{
    public:
        TestCommand(const std::deque<std::string>& args): Command(args){}
        ~TestCommand() = default;

        DECLARE_COMMAND(Test);
    };
}

#endif //TOKEN_COMMAND_H
#pragma clang diagnostic pop