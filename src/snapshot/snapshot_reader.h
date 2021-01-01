#ifndef TOKEN_SNAPSHOT_READER_H
#define TOKEN_SNAPSHOT_READER_H

#include "snapshot/section.h"
#include "snapshot/snapshot.h"
#include "utils/file_reader.h"
#include "utils/crash_report.h"

namespace Token{
  class Snapshot;
  class SnapshotReader : public BinaryFileReader{
    friend class Snapshot;
   public:
    SnapshotReader(const std::string& filename):
      BinaryFileReader(filename){}
    ~SnapshotReader() = default;

    SnapshotPtr ReadSnapshot(){
      int64_t start = GetCurrentPosition();
      if(!SnapshotSection::IsPrologueSection(ReadUnsignedLong())){
        LOG(WARNING) << "cannot read snapshot prologue section @" << start << " in: " << GetFilename();
        return SnapshotPtr(nullptr);
      }
      SnapshotPrologueSection prologue(this);

      start = GetCurrentPosition();
      if(!SnapshotSection::IsBlockChainSection(ReadUnsignedLong())){
        LOG(WARNING) << "cannot read snapshot block chain section @" << start << " in: " << GetFilename();
        return SnapshotPtr(nullptr);
      }
      SnapshotBlockChainSection blocks(this);
      return std::make_shared<Snapshot>(GetFilename(), prologue, blocks);
    }
  };
}

#endif //TOKEN_SNAPSHOT_READER_H