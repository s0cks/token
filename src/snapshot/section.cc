#include "snapshot/section.h"
#include "snapshot/snapshot_writer.h"
#include "snapshot/snapshot_reader.h"

namespace Token{
  SnapshotPrologueSection::SnapshotPrologueSection(SnapshotReader* reader):
    SnapshotSection(Type::kPrologue),
    timestamp_(reader->ReadLong()),
    version_(reader->ReadVersion()){}

  bool SnapshotPrologueSection::Write(SnapshotWriter* writer) const{
    writer->WriteLong(timestamp_);
    writer->WriteVersion(version_);
    return true;
  }

  SnapshotBlockChainSection::SnapshotBlockChainSection(SnapshotReader* reader):
    SnapshotSection(SnapshotSection::kBlockChain),
    head_(reader->ReadHash()){}

  bool SnapshotBlockChainSection::Write(SnapshotWriter* writer) const{
    writer->WriteHash(head_);
    return true;
  }
}