#include "snapshot.h"
#include "snapshot_file.h"

namespace Token{
    bool Snapshot::WriteSnapshot(Snapshot* snapshot){
        SnapshotWriter writer;
        return writer.WriteSnapshot();
    }

    bool Snapshot::WriteNewSnapshot(){
        Snapshot snapshot;
        return WriteSnapshot(&snapshot);
    }

    Snapshot* Snapshot::ReadSnapshot(const std::string& filename){
        SnapshotReader reader(filename);
        return reader.ReadSnapshot();
    }
}