#ifndef TOKEN_SNAPSHOT_READER_H
#define TOKEN_SNAPSHOT_READER_H

#include "snapshot_file.h"

namespace Token{
    class SnapshotReader{
    private:
        SnapshotFile file_;
    public:
        SnapshotReader(const std::string& filename):
                file_(filename){}
        ~SnapshotReader() = default;

        SnapshotFile* GetFile(){
            return &file_;
        }

        Snapshot* ReadSnapshot();
    };
}

#endif //TOKEN_SNAPSHOT_READER_H