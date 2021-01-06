#include "utils/bitfield.h"
#include "snapshot/snapshot.h"

namespace Token{
  bool Snapshot::WriteNewSnapshot(){
    CheckSnapshotDirectory();
    SnapshotWriter writer;
    return writer.WriteSnapshot();
  }

  SnapshotPtr Snapshot::ReadSnapshot(const std::string& filename){
    SnapshotReader reader(filename);
    return reader.ReadSnapshot();
  }
}