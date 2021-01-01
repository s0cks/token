#include "utils/bitfield.h"
#include "snapshot/snapshot.h"
#include "snapshot_writer.h"
#include "snapshot_reader.h"

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