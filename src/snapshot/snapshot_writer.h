#ifndef TOKEN_SNAPSHOT_WRITER_H
#define TOKEN_SNAPSHOT_WRITER_H

#include "snapshot/section.h"
#include "snapshot/snapshot.h"
#include "utils/file_writer.h"
#include "utils/crash_report.h"

namespace Token{
  class SnapshotWriter : public BinaryFileWriter{
   private:
    static inline std::string
    GetNewSnapshotFilename(){
      std::stringstream filename;
      filename << Snapshot::GetSnapshotDirectory();
      filename << "/snapshot-" << GetTimestampFormattedFileSafe(GetCurrentTimestamp()) << ".dat";
      return filename.str();
    }

    void WriteHeader(const SnapshotSectionHeader& header, int64_t pos);

    inline void
    WriteHeader(const SnapshotSectionHeader& header){
      WriteUnsignedLong(header);
    }

    template<class T>
    bool WriteSection(const T& section){
      WriteHeader(section.GetSectionHeader());
      section.Write(this);
      return true;
    }
   public:
    SnapshotWriter(const std::string& filename = GetNewSnapshotFilename()):
      BinaryFileWriter(filename){}
    ~SnapshotWriter() = default;

    bool WriteSnapshot();
  };
}

#endif //TOKEN_SNAPSHOT_WRITER_H