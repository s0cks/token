#ifndef TOKEN_SNAPSHOT_WRITER_H
#define TOKEN_SNAPSHOT_WRITER_H

#include "snapshot.h"
#include "file_writer.h"
#include "crash_report.h"

namespace Token{
    class SnapshotWriter : public BinaryFileWriter{
        friend class SnapshotSection;
    private:
        Snapshot* snapshot_; // TODO: remove snapshot_ instance

        static inline std::string
        GetNewSnapshotFilename(){
            std::stringstream filename;
            filename << Snapshot::GetSnapshotDirectory();
            filename << "/snapshot-" << GetTimestampFormattedFileSafe(GetCurrentTimestamp()) << ".dat";
            return filename.str();
        }

        void WriteHeader(SnapshotSection* section);
        void UpdateHeader(SnapshotSection* section);
    public:
        SnapshotWriter(Snapshot* snapshot, const std::string& filename=GetNewSnapshotFilename()):
            BinaryFileWriter(filename),
            snapshot_(snapshot){}
        ~SnapshotWriter() = default;

        Snapshot* GetSnapshot() const{
            return snapshot_;
        }

        bool WriteSnapshot();
        void WriteReference(const IndexReference& ref);
    };
}

#endif //TOKEN_SNAPSHOT_WRITER_H