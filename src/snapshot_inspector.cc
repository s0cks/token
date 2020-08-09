#include "snapshot_inspector.h"
#include "snapshot_loader.h"

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
        LOG(INFO) << "  - Hash: " << blk->GetSHA256Hash();
        LOG(INFO) << "  - Previous Hash: " << blk->GetPreviousHash();
        LOG(INFO) << "  - Merkle Root: " << blk->GetMerkleRoot();
        LOG(INFO) << "  - Transactions: ";
        for(size_t idx = 0; idx < blk->GetNumberOfTransactions(); idx++){
            Handle<Transaction> tx = blk->GetTransaction(idx);
            LOG(INFO) << "      * #" << tx->GetIndex() << ": " << tx->GetHash();
        }
    }

    void SnapshotInspector::SetSnapshot(Snapshot* snapshot){
        if(HasSnapshot()){
            delete snapshot_;
            delete blk_loader_;
        }

        snapshot_ = snapshot;
        blk_loader_ = snapshot ?
                        new SnapshotBlockLoader(snapshot) :
                        nullptr;
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
        LOG(INFO) << "Hello World";
    }

    void SnapshotInspector::HandleGetBlockCommand(Token::SnapshotInspectorCommand* cmd){
        uint256_t hash = cmd->GetNextArgumentHash();
        PrintBlock(GetBlock(hash));
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