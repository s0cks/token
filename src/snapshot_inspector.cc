#include "snapshot_inspector.h"

namespace Token{
    static void
    AllocBuffer(uv_handle_t* handle, size_t suggested_size, uv_buf_t* buff){
        buff->base = (char*)malloc(4096);
        buff->len = 4096;
    }

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
        LOG(INFO) << "Created: " << GetTimestampFormattedReadable(snapshot->prologue_.GetTimestamp());
        LOG(INFO) << "Version: " << snapshot->prologue_.GetVersion();
        LOG(INFO) << "Size: " << GetFilesize(snapshot->GetFilename()) << " Bytes";
        LOG(INFO) << "Number of Blocks: " << snapshot->blocks_.GetNumberOfReferences();
        LOG(INFO) << "Number of Unclaimed Transactions: " << snapshot->utxos_.GetNumberOfReferences();
    }

    void SnapshotInspector::SetSnapshot(Snapshot* snapshot){
        if(HasSnapshot()) delete snapshot_;
        snapshot_ = snapshot;
    }

    void SnapshotInspector::Inspect(Snapshot* snapshot){
        SetSnapshot(snapshot);
        uv_loop_t* loop = uv_loop_new();

        uv_loop_init(loop);
        uv_pipe_init(loop, &stdin_, 0);
        uv_pipe_open(&stdin_, 0);
        uv_pipe_init(loop, &stdout_, 0);
        uv_pipe_open(&stdout_, 1);

        int err;
        if((err = uv_read_start((uv_stream_t*)&stdin_, &AllocBuffer, &OnCommandReceived)) != 0){
            LOG(WARNING) << "couldn't start reading from stdin-pipe: " << uv_strerror(err);
            return; //TODO: shutdown properly
        }

        uv_run(loop, UV_RUN_DEFAULT);
        uv_read_stop((uv_stream_t*)&stdin_);
        uv_loop_close(loop);
        SetSnapshot(nullptr);
    }

    void SnapshotInspector::OnCommandReceived(uv_stream_t* stream, ssize_t nread, const uv_buf_t* buff){
        SnapshotInspector* inspector = (SnapshotInspector*)stream->data;

        if(nread == UV_EOF){
            LOG(ERROR) << "client disconnected!";
            return;
        } else if(nread < 0){
            LOG(ERROR) << "[" << nread << "] client read error: " << std::string(uv_strerror(nread));
            return;
        } else if(nread == 0){
            LOG(WARNING) << "zero message size received";
            return;
        } else if(nread >= 4096){
            LOG(ERROR) << "too large of a buffer";
            return;
        }

        std::string line(buff->base, nread - 1);

        std::deque<std::string> args;
        SplitString(line, args, ' ');

        std::string name = args.front();
        args.pop_front();

        SnapshotInspectorCommand cmd(name, args);
        switch(cmd.GetType()){
#define DECLARE_HANDLE(Name, Text, Parameters) \
            case SnapshotInspectorCommand::Type::k##Name##Type: \
                inspector->Handle##Name##Command(&cmd); \
                break;
            FOR_EACH_INSPECTOR_COMMAND(DECLARE_HANDLE);
            case SnapshotInspectorCommand::Type::kUnknownType:
            default:
                LOG(WARNING) << "unable to handle command: " << cmd;
                return;
        }
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
            snapshot_->Accept(&printer);
        } else{
            LOG(INFO) << "Unclaimed Transactions: ";
            SnapshotUnclaimedTransactionPrinter printer;
            snapshot_->Accept(&printer);
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

    std::string SnapshotInspectorCommand::GetCommandName() const{
        switch(GetType()){
#define DECLARE_NAME(Name, Text, Parameters) \
            case SnapshotInspectorCommand::Type::k##Name##Type: return #Text;
            FOR_EACH_INSPECTOR_COMMAND(DECLARE_NAME);
            case SnapshotInspectorCommand::Type::kUnknownType:
            default:
                return "Unknown";
        }
    }
}