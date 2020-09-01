#include "token.h"
#include "common.h"
#include "command.h"
#include "allocator.h"

#include "snapshot_inspector.h"
#include "heap_dump_inspector.h"

static inline void
InitializeLogging(char* arg0){
    using namespace Token;
    google::LogToStderr();
    google::InitGoogleLogging(arg0);
}

DEFINE_string(inspector_tool, "", "The inspector tool to use");

int
main(int argc, char** argv){
    using namespace Token;
    SignalHandlers::Initialize();
    gflags::ParseCommandLineFlags(&argc, &argv, true);
    InitializeLogging(argv[0]);
    Allocator::Initialize();

    std::string inspector_path;
    if(FLAGS_inspector_tool == "snapshot"){
        LOG(INFO) << "Please enter a snapshot to inspect (type .exit to quit):";
        std::cin >> inspector_path;

        if(inspector_path == ".exit") return EXIT_SUCCESS;
        if(!FileExists(inspector_path)){
            LOG(WARNING) << "Snapshot " << inspector_path << " doesn't exist, please enter a valid snapshot path";
            return EXIT_FAILURE;
        }

        SnapshotInspector inspector;
        Snapshot* snapshot = Snapshot::ReadSnapshot(inspector_path);
        inspector.Inspect(snapshot);
    } else if(FLAGS_inspector_tool == "heap-dump"){
        LOG(INFO) << "Please enter a valid heap dump to inspect (type .exit to quit):";
        std::cin >> inspector_path;

        if(inspector_path == ".exit") return EXIT_SUCCESS;
        if(!FileExists(inspector_path)){
            LOG(WARNING) << "Heap Dump " << inspector_path << " doesn't exist, please enter a valid heap dump path";
            return EXIT_FAILURE;
        }

        HeapDumpInspector inspector;
        HeapDump* dump = HeapDump::ReadHeapDump(inspector_path);
        inspector.Inspect(dump);
    }
}