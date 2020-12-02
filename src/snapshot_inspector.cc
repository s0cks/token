#include "snapshot_inspector.h"

namespace Token{
    static inline void
    PrintBlock(Block* blk){
        LOG(INFO) << "Block #" << blk->GetHeight();
        LOG(INFO) << "  - Timestamp: " << blk->GetTimestamp();
        LOG(INFO) << "  - Height: " << blk->GetHeight();
        LOG(INFO) << "  - Hash: " << blk->GetHash();
        LOG(INFO) << "  - Previous Hash: " << blk->GetPreviousHash();
        LOG(INFO) << "  - Merkle Root: " << blk->GetMerkleRoot();
        LOG(INFO) << "  - Transactions: ";

        int64_t idx;
        for(idx = 0;
            idx < blk->GetNumberOfTransactions();
            idx++){
            Transaction& tx = blk->transactions()[idx];
            LOG(INFO) << "      * #" << tx.GetIndex() << ": " << tx.GetHash();
        }
    }

    static inline void
    PrintUnclaimedTransaction(UnclaimedTransaction* utxo){
        LOG(INFO) << "  - " << utxo << " => " << utxo->GetUser();
    }

    void SnapshotInspector::PrintSnapshot(Snapshot* snapshot){
        LOG(INFO) << "Snapshot: " << snapshot->GetFilename();
        LOG(INFO) << "Created: " << GetTimestampFormattedReadable(snapshot->GetTimestamp());
        LOG(INFO) << "Version: " << snapshot->GetVersion();
        LOG(INFO) << "Size: " << GetFilesize(snapshot->GetFilename()) << " Bytes";
    }

    void SnapshotInspector::HandleStatusCommand(Token::SnapshotInspectorCommand* cmd){
        PrintSnapshot(GetSnapshot());
    }

    void SnapshotInspector::HandleGetDataCommand(Token::SnapshotInspectorCommand* cmd){
        //TODO: PrintBlock(GetBlock(Hash));
    }

    class SnapshotBlockPrinter : public SnapshotVisitor{
    public:
        SnapshotBlockPrinter() = default;
        ~SnapshotBlockPrinter() = default;

        bool Visit(Block* blk){
            PrintBlock(blk);
            return true;
        }
    };

    void SnapshotInspector::HandleGetBlocksCommand(Token::SnapshotInspectorCommand* cmd){
        LOG(INFO) << "Blocks:";
        SnapshotBlockPrinter printer;
        GetSnapshot()->Accept(&printer);
    }
}