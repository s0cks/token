#include "snapshot_inspector.h"

namespace Token{
    static inline void
    PrintBlock(const Handle<Block>& blk){
        LOG(INFO) << "Block #" << blk->GetHeight();
        LOG(INFO) << "  - Timestamp: " << blk->GetTimestamp();
        LOG(INFO) << "  - Height: " << blk->GetHeight();
        LOG(INFO) << "  - Hash: " << blk->GetHash();
        LOG(INFO) << "  - Previous Hash: " << blk->GetPreviousHash();
        LOG(INFO) << "  - Merkle Root: " << blk->GetMerkleRoot();
        LOG(INFO) << "  - Transactions: ";
        for(intptr_t idx = 0; idx < blk->GetNumberOfTransactions(); idx++){
            Handle<Transaction> tx = blk->GetTransaction(idx);
            LOG(INFO) << "      * #" << tx->GetIndex() << ": " << tx->GetHash();
        }
    }

    static inline void
    PrintUnclaimedTransaction(const Handle<UnclaimedTransaction>& utxo){
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
        //TODO: PrintBlock(GetBlock(hash));
    }

    class SnapshotBlockPrinter : public SnapshotVisitor{
    public:
        SnapshotBlockPrinter() = default;
        ~SnapshotBlockPrinter() = default;

        bool Visit(const Handle<Block>& blk){
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