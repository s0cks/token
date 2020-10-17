#include "snapshot_reader.h"

namespace Token{
    Snapshot* SnapshotReader::ReadSnapshot() {
        Snapshot* snapshot = new Snapshot(GetFilename());
        do{
            SnapshotSectionHeader header = ReadSectionHeader();
            switch(GetSnapshotSection(header)){
                case SnapshotSection::kSnapshotPrologue:{
                    LOG(INFO) << "reading snapshot prologue....";
                    snapshot->SetTimestamp(ReadUnsignedLong());
                    snapshot->SetVersion(ReadString());
                    snapshot->SetHead(ReadHash());
                    break;
                }
                case SnapshotSection::kSnapshotBlockChainData:{
                    LOG(INFO) << "reading snapshot block chain....";
                    uint64_t num_blocks = ReadUnsignedInt();
                    snapshot->SetNumberOfBlocks(num_blocks);
                    LOG(INFO) << "reading " << num_blocks << " blocks from snapshot....";
                    for(uint64_t idx = 0; idx < num_blocks; idx++)
                        //TODO: snapshot->SetBlock(idx, ReadObject().CastTo<Block>());
                    std::sort(snapshot->blocks_, snapshot->blocks_+num_blocks, Block::HeightComparator());
                    return snapshot;
                }
                case SnapshotSection::kSnapshotUnclaimedTransactionPoolData:
                default:{
                    LOG(WARNING) << "couldn't decode section " << header;
                    delete snapshot;
                    return nullptr;
                }
            }
        } while(true);
    }
}