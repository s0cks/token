#ifndef TOKEN_HEAP_DUMP_INSPECTOR_H
#define TOKEN_HEAP_DUMP_INSPECTOR_H

#include <uv.h>
#include "command.h"
#include "heap_dump.h"
#include "inspector.h"

#define FOR_EACH_INSPECTOR_COMMAND(V) \
    V(Status, ".status", 0) \
    V(GetHeap, "getheap", 1)

namespace Token{
    class HeapDumpInspectorCommand : public Command{
    public:
        HeapDumpInspectorCommand(std::deque<std::string>& args):
            Command(args){}
        ~HeapDumpInspectorCommand() = default;

#define DEFINE_TYPE_CHECK(Name, Text, ArgumentCount) \
        bool Is##Name##Command() const{ return !strncmp(name_.data(), Text, strlen(Text)); }
        FOR_EACH_INSPECTOR_COMMAND(DEFINE_TYPE_CHECK)
#undef DEFINE_TYPE_CHECK
    };

    class HeapDumpInspector : public Inspector<HeapDump, HeapDumpInspectorCommand>{
    private:
#define DEFINE_COMMAND_HANDLER(Name, Text, ArgumentCount) \
        void Handle##Name##Command(HeapDumpInspectorCommand* cmd);
        FOR_EACH_INSPECTOR_COMMAND(DEFINE_COMMAND_HANDLER)
#undef DEFINE_COMMAND_HANDLER

        void OnCommand(HeapDumpInspectorCommand* cmd){
#define DEFINE_CHECK(Name, Text, ArgumentCount) \
            if(cmd->Is##Name##Command()){ \
                Handle##Name##Command(cmd); \
                return; \
            }
            FOR_EACH_INSPECTOR_COMMAND(DEFINE_CHECK)
#undef DEFINE_CHECK
        }
    public:
        HeapDumpInspector():
            Inspector(){}
        ~HeapDumpInspector() = default;
    };
}

#undef FOR_EACH_INSPECTOR_COMMAND
#endif //TOKEN_HEAP_DUMP_INSPECTOR_H