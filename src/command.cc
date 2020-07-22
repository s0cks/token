#include "command.h"

namespace Token{
    Command* Command::ParseCommand(const std::string& line){
        std::deque<std::string> args;
        SplitString(line, args, ' ');

        std::string command = args.front();
        args.pop_front();

#define DECLARE_CHECK(Name, Text, Parameters) \
    if((strncmp(command.c_str(), (Text), strlen((Text))) == 0) && (Parameters) == args.size()){ \
        return new Name##Command(args); \
    }
FOR_EACH_COMMAND(DECLARE_CHECK);
#undef DECLARE_CHECK
        return nullptr; // couldn't parse
    }
}