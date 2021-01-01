#include "snapshot/section.h"
#include "snapshot/snapshot.h"

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

  SnapshotBlockChainSection::SnapshotBlockChainSection():
    SnapshotSection(SnapshotSection::kBlockChain),
    references_(){}

  SnapshotBlockChainSection::SnapshotBlockChainSection(SnapshotReader* reader):
    SnapshotSection(SnapshotSection::kBlockChain),
    references_(){
  }

  bool SnapshotBlockChainSection::Write(SnapshotWriter* writer) const{
    //TODO: write block chain section
    return true;
  }
}