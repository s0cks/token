#ifndef TOKEN_SNAPSHOT_BLOCK_CHAIN_H
#define TOKEN_SNAPSHOT_BLOCK_CHAIN_H

#include "snapshot.h"
#include "snapshot_writer.h"
#include "snapshot_reader.h"

namespace Token{
    class SnapshotBlockChainSection : public IndexedSection{
        friend class Snapshot;
    protected:
        bool WriteIndexTable(SnapshotWriter* writer);
        bool WriteData(SnapshotWriter* writer);
        bool LinkIndexTable(SnapshotWriter* writer);
    public:
        SnapshotBlockChainSection(): IndexedSection(SnapshotSection::kBlockChainDataSection){}
        ~SnapshotBlockChainSection() = default;

        void operator=(const SnapshotBlockChainSection& section){
            IndexedSection::operator=(section);
        }
    };
}

#endif //TOKEN_SNAPSHOT_BLOCK_CHAIN_H