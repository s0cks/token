#include "core/include/common.h"
#include "core/include/command.h"
#include "snapshot/snapshot_inspector.h"

static inline void
InitializeLogging(char *arg0){
  using namespace token;
  google::LogToStderr();
  google::InitGoogleLogging(arg0);
}

DEFINE_string(inspector_tool, "", "The inspector tool to use");

int
main(int argc, char **argv){
  using namespace token;
  SignalHandlers::Initialize();
  gflags::ParseCommandLineFlags(&argc, &argv, true);
  InitializeLogging(argv[0]);

  std::string inspector_path;
  if(FLAGS_inspector_tool == "snapshot"){
    LOG(INFO) << "Please enter a snapshot to inspect (type .exit to quit):";
    std::cin >> inspector_path;

    if(inspector_path == ".exit") return EXIT_SUCCESS;
    if(!FileExists(inspector_path)){
      LOG(WARNING) << "Snapshot " << inspector_path << " doesn't exist, please enter a IsValid snapshot path";
      return EXIT_FAILURE;
    }

    SnapshotInspector inspector;
    SnapshotPtr snapshot = Snapshot::ReadSnapshot(inspector_path);
    inspector.Inspect(snapshot.get());
  } else if(FLAGS_inspector_tool == "heap-dump"){
    LOG(INFO) << "Please enter a heap dump to inspect (type .exit to quit):";
    std::cin >> inspector_path;

    if(inspector_path == ".exit") return EXIT_SUCCESS;
    if(!FileExists(inspector_path)){
      LOG(ERROR) << "Heap Dump " << inspector_path << " doesn't exist, please enter a IsValid path";
      return EXIT_FAILURE;
    }
  }

  return EXIT_SUCCESS;
}