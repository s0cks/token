#ifndef TOKEN_SNAPSHOT_INSPECTOR_H
#define TOKEN_SNAPSHOT_INSPECTOR_H

#include <uv.h>
#include "snapshot.h"

namespace Token{
#define FOR_EACH_INSPECTOR_COMMAND(V) \
    V(Status, ".status", 0) \
    V(GetData, "getdata", 1) \
    V(GetBlocks, "getblocks", 0)

    class SnapshotInspectorCommand;
    class SnapshotInspector{
    private:
        uv_pipe_t stdin_;
        uv_pipe_t stdout_;

        Snapshot* snapshot_;

        void SetSnapshot(Snapshot* snapshot);
        void PrintSnapshot(Snapshot* snapshot);

        inline Snapshot*
        GetSnapshot() const{
            return snapshot_;
        }

        inline bool
        HasSnapshot() const{
            return GetSnapshot() != nullptr;
        }

        inline Handle<Block>
        GetBlock(const uint256_t& hash){
            return GetSnapshot()->GetBlock(hash);
        }

        static void OnCommandReceived(uv_stream_t* stream, ssize_t nread, const uv_buf_t* buff);

#define DECLARE_HANDLER(Name, Text, Parameters) \
        void Handle##Name##Command(SnapshotInspectorCommand* cmd);
        FOR_EACH_INSPECTOR_COMMAND(DECLARE_HANDLER);
#undef DECLARE_HANDLER
    public:
        SnapshotInspector():
            stdin_(),
            stdout_(),
            snapshot_(nullptr){
            stdin_.data = this;
            stdout_.data = this;
        }
        ~SnapshotInspector() = default;

        void Inspect(Snapshot* snapshot);
    };

    class SnapshotInspectorCommand{
        friend class SnapshotInspector;
    public:
        enum class Type{
            kUnknownType=0,
#define DECLARE_TYPE(Name, Text, Parameters) k##Name##Type,
            FOR_EACH_INSPECTOR_COMMAND(DECLARE_TYPE)
#undef DECLARE_TYPE
        };
    private:
        Type type_;
        std::deque<std::string> args_;

        SnapshotInspectorCommand(Type type, std::deque<std::string>& args):
            type_(type),
            args_(args){}
        SnapshotInspectorCommand(const std::string& name, std::deque<std::string>& args):
            type_(GetCommand(name)),
            args_(args){}
        SnapshotInspectorCommand(std::deque<std::string>& args):
            type_(GetCommand(args)),
            args_(args){}

        inline Type
        GetType() const{
            return type_;
        }

        static inline Type
        GetCommand(const std::string& name){
#define DECLARE_CHECK(Name, Text, Parameter) \
            if(!strncmp(name.c_str(), (Text), strlen((Text)))) {\
                return Type::k##Name##Type; \
            }
            FOR_EACH_INSPECTOR_COMMAND(DECLARE_CHECK);
#undef DECLARE_CHECK
            return Type::kUnknownType;
        }

        static inline Type
        GetCommand(std::deque<std::string>& args){
            std::string name = args.front();
            args.pop_front();
            return GetCommand(name);
        }
    public:
        ~SnapshotInspectorCommand() = default;

        std::string GetCommandName() const;

#define DECLARE_CHECK(Name, Text, Parameter) \
        bool Is##Name##Command() const{ return GetType() == SnapshotInspectorCommand::Type::k##Name##Type; }
        FOR_EACH_INSPECTOR_COMMAND(DECLARE_CHECK);
#undef DECLARE_CHECK

        std::string GetNextArgument(){
            std::string arg = args_.front();
            args_.pop_front();
            return arg;
        }

        uint256_t GetNextArgumentHash(){
            return HashFromHexString(GetNextArgument());
        }

        uint32_t GetNextArgumentUInt32(){
            std::string arg = GetNextArgument();
            return (uint32_t)(atol(arg.c_str()));
        }

        friend std::ostream& operator<<(std::ostream& stream, const SnapshotInspectorCommand& cmd){
            std::vector<std::string> args(cmd.args_.begin(), cmd.args_.end());
            stream << cmd.GetCommandName();
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

#endif //TOKEN_SNAPSHOT_INSPECTOR_H