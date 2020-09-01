#include "snapshot_inspector.h"
#include "snapshot_prologue.h"
#include "snapshot_block_chain.h"
#include "snapshot_unclaimed_transaction_pool.h"

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
        for(size_t idx = 0; idx < blk->GetNumberOfTransactions(); idx++){
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
        LOG(INFO) << "Created: " << GetTimestampFormattedReadable(snapshot->GetPrologueSection()->GetTimestamp());
        LOG(INFO) << "Version: " << snapshot->GetPrologueSection()->GetVersion();
        LOG(INFO) << "Size: " << GetFilesize(snapshot->GetFilename()) << " Bytes";
        LOG(INFO) << "Number of Blocks: " << snapshot->GetBlockChainSection()->GetNumberOfReferences();
        LOG(INFO) << "Number of Unclaimed Transactions: " << snapshot->GetUnclaimedTransactionPoolSection()->GetNumberOfReferences();
    }

    void SnapshotInspector::HandleStatusCommand(Token::SnapshotInspectorCommand* cmd){
        PrintSnapshot(GetSnapshot());
    }

    void SnapshotInspector::HandleGetDataCommand(Token::SnapshotInspectorCommand* cmd){
        uint256_t hash = cmd->GetNextArgumentHash();
        PrintBlock(GetBlock(hash));
    }

    class SnapshotUnclaimedTransactionPrinter : public SnapshotUnclaimedTransactionDataVisitor{
    private:
        std::string user_;
    public:
        SnapshotUnclaimedTransactionPrinter(const std::string& user):
            SnapshotUnclaimedTransactionDataVisitor(),
            user_(user){}
        SnapshotUnclaimedTransactionPrinter():
            SnapshotUnclaimedTransactionDataVisitor(),
            user_(){}
        ~SnapshotUnclaimedTransactionPrinter() = default;

        std::string GetTarget() const{
            return user_;
        }

        bool Visit(const Handle<UnclaimedTransaction>& utxo){
            if(utxo->GetUser() == GetTarget()) PrintUnclaimedTransaction(utxo);
            return true;
        }
    };

    void SnapshotInspector::HandleGetUnclaimedTransactionsCommand(Token::SnapshotInspectorCommand* cmd){
        if(cmd->HasNextArgument()){
            std::string user = cmd->GetNextArgument();
            LOG(INFO) << "Unclaimed Transactions (" << user << "):";
            SnapshotUnclaimedTransactionPrinter printer(user);
            GetSnapshot()->Accept(&printer);
        } else{
            LOG(INFO) << "Unclaimed Transactions: ";
            SnapshotUnclaimedTransactionPrinter printer;
            GetSnapshot()->Accept(&printer);
        }
    }

    class SnapshotBlockPrinter : public SnapshotBlockDataVisitor{
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