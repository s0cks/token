#ifndef TOKEN_SNAPSHOT_WRITER_H
#define TOKEN_SNAPSHOT_WRITER_H

#include "snapshot.h"
#include "utils/file_writer.h"
#include "crash_report.h"

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

        inline void
        WriteHeader(SnapshotSectionHeader header){
            WriteUnsignedLong(header);
        }

        bool WriteHeaderAt(int64_t pos, SnapshotSectionHeader header);
    public:
        SnapshotWriter(const std::string& filename=GetNewSnapshotFilename()):
            BinaryFileWriter(filename){}
        ~SnapshotWriter() = default;

        bool WriteSnapshot();
    };
}

#endif //TOKEN_SNAPSHOT_WRITER_H