#include "snapshot_reader.h"
#include "snapshot_prologue.h"
#include "snapshot_block_chain.h"
#include "snapshot_unclaimed_transaction_pool.h"

namespace Token{
    IndexReference SnapshotReader::ReadReference(){
        uint256_t hash = ReadHash();
        size_t size = ReadInt();
        int64_t index_pos = ReadLong();
        int64_t data_pos = ReadLong();
        return IndexReference(hash, size, index_pos, data_pos);
    }

    void SnapshotReader::ReadHeader(Token::SnapshotSection* section){
        int64_t offset = GetCurrentPosition();
        int32_t id = ReadInt();
        int64_t size = ReadLong();
        section->SetRegion(offset, size);
        LOG(INFO) << "read section #" << id << " (" << size << ") @" << offset;
    }

    void SnapshotReader::SkipData(SnapshotSection* section){
        SetCurrentPosition(section->GetOffset() + section->GetSize());
    }

    Snapshot* SnapshotReader::ReadSnapshot() {
        Snapshot* snapshot = new Snapshot();
        snapshot->filename_ = GetFilename();
        {
            ReadHeader(snapshot->GetPrologueSection());
            snapshot->GetPrologueSection()->Accept(this);
        }
        {
            ReadHeader(snapshot->GetBlockChainSection());
            snapshot->GetBlockChainSection()->Accept(this);
            SkipData(snapshot->GetBlockChainSection());
        }
        {
            ReadHeader(snapshot->GetUnclaimedTransactionPoolSection());
            snapshot->GetUnclaimedTransactionPoolSection()->Accept(this);
        }
        return snapshot;
    }
}