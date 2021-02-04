#include "snapshot/snapshot_inspector.h"

namespace token{
  static inline void
  PrintBlock(const BlockPtr& blk){
    LOG(INFO) << "Block #" << blk->GetHeight();
    LOG(INFO) << "  - Timestamp: " << FormatTimestampReadable(blk->GetTimestamp());
    LOG(INFO) << "  - Height: " << blk->GetHeight();
    LOG(INFO) << "  - Hash: " << blk->GetHash();
    LOG(INFO) << "  - Previous Hash: " << blk->GetPreviousHash();
    LOG(INFO) << "  - Merkle Root: " << blk->GetMerkleRoot();
    LOG(INFO) << "  - Transactions: ";

    for(auto& tx : blk->transactions()){
      LOG(INFO) << "      * #" << tx->GetIndex() << ": " << tx->GetHash();
    }
  }

  static inline void
  PrintUnclaimedTransaction(UnclaimedTransaction* utxo){
    LOG(INFO) << "  - " << utxo << " => " << utxo->GetUser();
  }

  void SnapshotInspector::PrintSnapshot(Snapshot* snapshot){
    LOG(INFO) << "Snapshot: " << snapshot->GetFilename();
    LOG(INFO) << "Created: " << FormatTimestampReadable(snapshot->GetTimestamp());
    LOG(INFO) << "Version: " << snapshot->GetVersion();
    LOG(INFO) << "Size: " << GetFilesize(snapshot->GetFilename()) << " Bytes";
  }

  void SnapshotInspector::HandleStatusCommand(token::SnapshotInspectorCommand* cmd){
    PrintSnapshot(GetSnapshot());
  }

  void SnapshotInspector::HandleGetDataCommand(token::SnapshotInspectorCommand* cmd){
    //TODO: PrintBlock(GetBlock(Hash));
  }

  class SnapshotBlockPrinter : public SnapshotVisitor{
   public:
    SnapshotBlockPrinter() = default;
    ~SnapshotBlockPrinter() = default;

    bool Visit(Block* blk){
      PrintBlock(BlockPtr(blk));
      return true;
    }
  };

  void SnapshotInspector::HandleGetBlocksCommand(token::SnapshotInspectorCommand* cmd){
    LOG(INFO) << "Blocks:";
    SnapshotBlockPrinter printer;
    return; //TODO: fixme
  }
}