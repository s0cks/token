#ifndef TOKEN_SNAPSHOT_READER_H
#define TOKEN_SNAPSHOT_READER_H

#include "snapshot.h"
#include "file_reader.h"
#include "crash_report.h"

namespace Token{
    class Snapshot;
    class SnapshotReader : public BinaryFileReader{
        friend class Snapshot;
    private:
        void SkipData(SnapshotSection* section);
        void ReadHeader(SnapshotSection* section);
    public:
        SnapshotReader(const std::string& filename):
            BinaryFileReader(filename){}
        ~SnapshotReader() = default;

        Snapshot* ReadSnapshot();
        IndexReference ReadReference();
    };
}

#endif //TOKEN_SNAPSHOT_READER_H