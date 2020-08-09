#include "snapshot_writer.h"

namespace Token{
    bool SnapshotWriter::WriteSnapshot(){
        SnapshotPrologueSection prologue;
        if(!prologue.Accept(this)){
            LOG(WARNING) << "couldn't write prologue section to snapshot: " << GetFile()->GetFilename();
            return false;
        }

        {
            // Encode Block Chain Data to Snapshot
            SnapshotBlockIndex mappings;

            // Write Block Index
            SnapshotBlockChainIndexSection index(&mappings);
            if(!index.Accept(this)){
                LOG(WARNING) << "couldn't write block chain index section to snapshot: " << GetFile()->GetFilename();
                return false;
            }

            // Write Block Data
            SnapshotBlockChainDataSection data(&mappings);
            if(!data.Accept(this)){
                LOG(WARNING) << "couldn't write block chain data section to snapshot: " << GetFile()->GetFilename();
                return false;
            }

            SnapshotBlockIndexLinker linker(&mappings);
            if(!linker.Accept(this)){
                LOG(WARNING) << "couldn't link the block chain index and data sections in snapshot: " << GetFile()->GetFilename();
                return false;
            }
        }
        return true;
    }
}