#ifndef TOKEN_SNAPSHOT_INSPECTOR_H
#define TOKEN_SNAPSHOT_INSPECTOR_H

#include <uv.h>
#include "command.h"
#include "snapshot.h"
#include "inspector.h"

namespace Token{
#define FOR_EACH_INSPECTOR_COMMAND(V) \
    V(Status, ".status", 0) \
    V(GetData, "getdata", 1) \
    V(GetBlocks, "getblocks", 0) \

  class SnapshotInspectorCommand : public Command{
   public:
    SnapshotInspectorCommand(std::deque<std::string> &args):
        Command(args){}
    ~SnapshotInspectorCommand() = default;

#define DEFINE_TYPE_CHECK(Name, Text, ArgumentCount) \
        bool Is##Name##Command() const{ return !strncmp(name_.data(), Text, strlen(Text)); }
    FOR_EACH_INSPECTOR_COMMAND(DEFINE_TYPE_CHECK)
#undef DEFINE_TYPE_CHECK
  };

  class SnapshotInspector : public Inspector<Snapshot, SnapshotInspectorCommand>{
   private:
    void PrintSnapshot(Snapshot *snapshot);

    inline Snapshot *
    GetSnapshot() const{
      return (Snapshot *) data_;
    }

    inline bool
    HasSnapshot() const{
      return GetSnapshot() != nullptr;
    }

#define DECLARE_HANDLER(Name, Text, Parameters) \
        void Handle##Name##Command(SnapshotInspectorCommand* cmd);
    FOR_EACH_INSPECTOR_COMMAND(DECLARE_HANDLER);
#undef DECLARE_HANDLER

    void OnCommand(SnapshotInspectorCommand *cmd){
#define DECLARE_CHECK(Name, Text, ArgumentCount) \
            if(cmd->Is##Name##Command()){ \
                Handle##Name##Command(cmd); \
                return; \
            }
      FOR_EACH_INSPECTOR_COMMAND(DECLARE_CHECK);
#undef DECLARE_CHECK
    }
   public:
    SnapshotInspector():
        Inspector(){}
    ~SnapshotInspector() = default;
  };
}

#endif //TOKEN_SNAPSHOT_INSPECTOR_H