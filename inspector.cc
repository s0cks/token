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

    LOG(INFO) << "please enter a snapshot to inspect:";
    std::string snapshot_path;
    std::cin >> snapshot_path;

    SnapshotInspector inspector;
    Snapshot* snapshot = Snapshot::ReadSnapshot(snapshot_path);
    inspector.Inspect(snapshot);
    return EXIT_SUCCESS;
}