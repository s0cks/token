#include "token.h"
#include "common.h"
#include "allocator.h"

#include "snapshot_inspector.h"

static inline void
InitializeLogging(char* arg0){
    using namespace Token;
    google::LogToStderr();
    google::InitGoogleLogging(arg0);
}

int
main(int argc, char** argv){
    using namespace Token;
    SignalHandlers::Initialize();
    gflags::ParseCommandLineFlags(&argc, &argv, true);
    InitializeLogging(argv[0]);
    Allocator::Initialize();

    do{
        LOG(INFO) << "Please enter a snapshot to inspect (type .exit to quit):";
        std::string snapshot_path;
        std::cin >> snapshot_path;

        if(snapshot_path == ".exit") break;

        if(!FileExists(snapshot_path)){
            LOG(WARNING) << "Snapshot " << snapshot_path << " doesn't exist!";
            continue;
        }

        SnapshotInspector inspector;
        Snapshot* snapshot = Snapshot::ReadSnapshot(snapshot_path);
        inspector.Inspect(snapshot);
    } while(true);
    return EXIT_SUCCESS;
}