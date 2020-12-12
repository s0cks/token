#ifndef TOKEN_SNAPSHOT_READER_H
#define TOKEN_SNAPSHOT_READER_H

#include "snapshot.h"
#include "utils/file_reader.h"
#include "crash_report.h"

namespace Token{
    class Snapshot;
    class SnapshotReader : public BinaryFileReader{
        friend class Snapshot;
    private:
        inline SnapshotSectionHeader
        ReadSectionHeader(){
            return ReadUnsignedLong();
        }
    public:
        SnapshotReader(const std::string& filename):
            BinaryFileReader(filename){}
        ~SnapshotReader() = default;

        Snapshot* ReadSnapshot();
    };
}

#endif //TOKEN_SNAPSHOT_READER_H