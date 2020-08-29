#include "snapshot_writer.h"
#include "snapshot_prologue.h"
#include "snapshot_block_chain.h"
#include "snapshot_unclaimed_transaction_pool.h"

namespace Token{
    void SnapshotWriter::WriteHeader(SnapshotSection* section){
        WriteInt(section->GetSectionId());
        WriteLong(section->GetSize());
    }

    void SnapshotWriter::UpdateHeader(Token::SnapshotSection* section){
        int64_t offset = section->GetOffset() + sizeof(uint32_t);

        int64_t last = GetCurrentPosition();
        SetCurrentPosition(offset);
        WriteLong(section->GetSize());
        SetCurrentPosition(last);
        LOG(INFO) << "wrote section #" << section->GetSectionId() << " (" << section->GetSize() << ") @" << section->GetOffset();
    }

    bool SnapshotWriter::WriteSnapshot(){
        Snapshot* snapshot = GetSnapshot();
        {
            int64_t offset = GetCurrentPosition();
            snapshot->GetPrologueSection()->SetRegion(offset, 0);
            WriteHeader(snapshot->GetPrologueSection());
            snapshot->GetPrologueSection()->Accept(this);
            int64_t size = (GetCurrentPosition() - offset);
            snapshot->GetPrologueSection()->SetRegion(offset, size);
            UpdateHeader(snapshot->GetPrologueSection());
        }

        {
            int64_t offset = GetCurrentPosition();
            WriteHeader(snapshot->GetBlockChainSection());
            snapshot->GetBlockChainSection()->Accept(this); //TODO: check result
            int64_t size = (GetCurrentPosition() - offset);
            snapshot->GetBlockChainSection()->SetRegion(offset, size);
            UpdateHeader(snapshot->GetBlockChainSection());
        }

        {
            int64_t offset = GetCurrentPosition();
            WriteHeader(snapshot->GetUnclaimedTransactionPoolSection());
            snapshot->GetUnclaimedTransactionPoolSection()->Accept(this); //TODO: check result
            int64_t size = (GetCurrentPosition() - offset);
            snapshot->GetUnclaimedTransactionPoolSection()->SetRegion(offset, size);
            UpdateHeader(snapshot->GetUnclaimedTransactionPoolSection());
        }
        return true;
    }



    void SnapshotWriter::WriteReference(const IndexReference& ref){
        WriteHash(ref.GetHash());
        WriteInt(ref.GetSize());
        WriteLong(ref.GetIndexPosition());
        WriteLong(ref.GetDataPosition());
    }
}