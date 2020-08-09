#include "snapshot_reader.h"

namespace Token{
    Snapshot* SnapshotReader::ReadSnapshot(){
        Snapshot* snapshot = new Snapshot(GetFile());

        SnapshotPrologueSection prologue(snapshot);
        if(!prologue.Accept(this)){
            LOG(WARNING) << "couldn't read prologue section from snapshot: " << GetFile()->GetFilename();
            delete snapshot;
            return nullptr;
        }

        SnapshotBlockChainIndexSection index(snapshot);
        if(!index.Accept(this)){
            LOG(WARNING) << "couldn't read block index section from snapshot: " << GetFile()->GetFilename();
            delete snapshot;
            return nullptr;
        }
        return snapshot;
    }
}