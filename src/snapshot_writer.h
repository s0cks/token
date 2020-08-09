#ifndef TOKEN_SNAPSHOT_WRITER_H
#define TOKEN_SNAPSHOT_WRITER_H

#include "snapshot_file.h"

namespace Token{
    class SnapshotWriter{
        friend class SnapshotSection;
    private:
        SnapshotFile file_;

        void WriteBlockChainData();
    public:
        SnapshotWriter(Snapshot* snapshot=new Snapshot()):
                file_(snapshot){}
        ~SnapshotWriter() = default;

        SnapshotFile* GetFile(){
            return &file_;
        }

        bool WriteSnapshot();
    };
}

#endif //TOKEN_SNAPSHOT_WRITER_H