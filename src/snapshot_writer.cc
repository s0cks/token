#include "snapshot_writer.h"

namespace Token{
    bool SnapshotWriter::WriteHeaderAt(int64_t pos, SnapshotSectionHeader header){
        int64_t last = GetCurrentPosition();
        SetCurrentPosition(pos);
        WriteHeader(header);
        SetCurrentPosition(last);
        return true;
    }

    bool SnapshotWriter::WriteSnapshot(){
        LOG(INFO) << "writing new snapshot...";
        {
            LOG(INFO) << "writing the prologue section....";
            // Encode the Prologue
            // ------------------
            // - u64: SectionHeader
            // - u64: GenerationTime
            // - u256: HeadHash
            // ------------------
            int64_t start = GetCurrentPosition();
            WriteHeader(CreateSnapshotSectionHeader(kSnapshotPrologue, 0)); // Header
            WriteUnsignedLong(GetCurrentTimestamp()); // Generation Time
            WriteString(GetVersion());
            WriteHash(BlockChain::GetHead()->GetHash()); // <HEAD>
            int64_t size = (GetCurrentPosition() - start);
            // update section header size
            WriteHeaderAt(start, CreateSnapshotSectionHeader(kSnapshotPrologue, size));
        }

        {
            LOG(INFO) << "writing the block chain section....";
            // Encode the Block Chain
            // ------------------------
            // - u64: SectionHeader
            // - u32: NumberOfBlocks
            // - block: Block #N
            // - block: Block #2
            // - block: Block #1
            // - block: Block #0
            // ------------------------
            int64_t start = GetCurrentPosition();
            WriteHeader(CreateSnapshotSectionHeader(kSnapshotBlockChainData, 0)); // SnapshotHeader
            WriteUnsignedInt(BlockChain::GetHead()->GetHeight());
            Handle<Block> blk = BlockChain::GetHead();
            while(true){
                WriteObject(blk);
                if(blk->IsGenesis()) break;
                blk = blk->GetPrevious().CastTo<Block>();
            }
            int64_t size = (GetCurrentPosition() - start);
            WriteHeaderAt(start, CreateSnapshotSectionHeader(kSnapshotBlockChainData, size));
        }

        {
            // Encode the Unclaimed Transaction Pool
            // ------------------------
            // - u64: SectionHeader
            // - u64: NumberOfBlocks
            // - block: Block #1
            // - block: Block #2
            // - block: Block #N
            // ------------------------
            int64_t offset = GetCurrentPosition();
            //TODO: write unclaimed transaction pool section
        }

        LOG(INFO) << "snapshot written to: " << GetFilename();
        return true;
    }
}