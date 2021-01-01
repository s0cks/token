#include "snapshot_writer.h"

namespace Token{
  void SnapshotWriter::WriteHeader(const SnapshotSectionHeader& header,int64_t pos){
    int64_t last = GetCurrentPosition();
    SetCurrentPosition(pos);
    WriteUnsignedLong(header);
    SetCurrentPosition(last);
  }

  bool SnapshotWriter::WriteSnapshot(){
    SnapshotPtr snapshot = Snapshot::NewInstance(GetFilename());
    LOG(INFO) << "writing new snapshot...";
    WriteSection(snapshot->GetPrologue());
    WriteSection(snapshot->GetBlockChain());
    LOG(INFO) << "snapshot written to: " << GetFilename();
    return true;
  }
}