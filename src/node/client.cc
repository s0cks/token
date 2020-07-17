#include "alloc/scope.h"
#include "node/node.h"
#include "node/task.h"
#include "node/client.h"

namespace Token{
    void NodeClient::OnSignal(uv_signal_t* handle, int signum){
        NodeClient* client = (NodeClient*)handle->data;
        uv_stop(client->loop_);
    }

    void NodeClient::Connect(const NodeAddress& addr){
        LOG(INFO) << "connecting to server: " << addr;
        SetState(State::kConnecting);
        SetPeer(addr);

        uv_loop_init(loop_);
        uv_signal_init(loop_, &sigterm_);
        uv_signal_start(&sigterm_, &OnSignal, SIGTERM);

        uv_signal_init(loop_, &sigint_);
        uv_signal_start(&sigterm_, &OnSignal, SIGINT);

        uv_tcp_init(loop_, &stream_);
        uv_tcp_keepalive(&stream_, 1, 60);

        struct sockaddr_in address;
        if(!addr.Get(&address)){
            LOG(WARNING) << "couldn't resolved address '" << addr;
            goto cleanup;
        }

        uv_pipe_init(loop_, &stdin_, 0);
        uv_pipe_open(&stdin_, 0);

        uv_pipe_init(loop_, &stdout_, 0);
        uv_pipe_open(&stdout_, 1);

        int err;
        if((err = uv_tcp_connect(&connection_, &stream_, (const struct sockaddr*)&address, &OnConnect)) != 0){
            LOG(WARNING) << "couldn't connect to peer: " << uv_strerror(err);
            goto cleanup;
        }

        uv_run(loop_, UV_RUN_DEFAULT);
        uv_signal_stop(&sigterm_);
        uv_signal_stop(&sigint_);
        uv_read_stop((uv_stream_t*)&stdin_);
    cleanup:
        uv_loop_close(loop_);
    }

    void NodeClient::OnConnect(uv_connect_t* conn, int status){
        if(status != 0){
            LOG(WARNING) << "error connecting to server: " << uv_strerror(status);
            return; //TODO: terminate connection
        }

        NodeClient* client = (NodeClient*)conn->data;

        int err;
        if((err = uv_read_start(client->GetStream(), &AllocBuffer, &OnMessageReceived)) != 0){
            LOG(ERROR) << " client read error: " << uv_strerror(err);
            return; //TODO: terminate connection
        }

        if((err = uv_read_start(client->GetStdinPipe(), &AllocBuffer, &OnCommandReceived)) != 0){
            LOG(ERROR) << "couldn't start reading from stdin-pipe: " << uv_strerror(err);
            return; //TODO: terminate connection
        }

        NodeInfo info(GenerateNonce(), NodeAddress());
        BlockHeader head;

        client->Send(VersionMessage::NewInstance(info, GenerateNonce(), head));
    }

    void NodeClient::OnMessageReceived(uv_stream_t* stream, ssize_t nread, const uv_buf_t* buff){
        NodeClient* session = (NodeClient*)stream->data;
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

        Scope scope;
        uint32_t offset = 0;
        std::vector<Message*> messages;
        do{
            uint32_t mtype = 0;
            memcpy(&mtype, &buff->base[offset + Message::kTypeOffset], Message::kTypeLength);

            uint64_t msize = 0;
            memcpy(&msize, &buff->base[offset + Message::kSizeOffset], Message::kSizeLength);

            Message* msg = nullptr;
            if(!(msg = Message::Decode(static_cast<Message::MessageType>(mtype), msize, (uint8_t*)&buff->base[offset + Message::kDataOffset]))){
                LOG(WARNING) << "couldn't decode message of type := " << mtype << " and of size := " << msize << ", at offset := " << offset;
                continue;
            }
            scope.Retain(msg);

            LOG(INFO) << "decoded message: " << msg->ToString();
            messages.push_back(msg);

            offset += (msize + Message::kHeaderSize);
        } while((offset + Message::kHeaderSize) < nread);

        for(size_t idx = 0; idx < messages.size(); idx++){
            Message* msg = messages[idx];
            HandleMessageTask* task = new HandleMessageTask(session, msg);
            switch(msg->GetMessageType()){
#define DEFINE_HANDLER_CASE(Name) \
            case Message::k##Name##MessageType: \
                session->Handle##Name##Message(task); \
                break;
                FOR_EACH_MESSAGE_TYPE(DEFINE_HANDLER_CASE);
#undef DEFINE_HANDLER_CASE
                case Message::kUnknownMessageType:
                default: //TODO: handle properly
                    break;
            }

            delete task;
        }
    }

    void NodeClient::HandleVersionMessage(HandleMessageTask* task){
        NodeClient* client = (NodeClient*)task->GetSession();
        VersionMessage* msg = (VersionMessage*)task->GetMessage();
        client->Send(VerackMessage::NewInstance(Node::GetInfo())); //TODO: fixme
    }

    void NodeClient::HandleVerackMessage(HandleMessageTask* task){
        NodeClient* client = (NodeClient*)task->GetSession();
        VerackMessage* msg = (VerackMessage*)task->GetMessage();

        client->SetState(State::kConnected);
        LOG(INFO) << "connected!";
    }

    void NodeClient::HandleBlockMessage(HandleMessageTask* task){
        NodeClient* client = (NodeClient*)task->GetSession();
        BlockMessage* msg = (BlockMessage*)task->GetMessage();

        Block* blk = msg->GetBlock();
        LOG(INFO) << "received block: " << blk->GetHeader();
    }

    void NodeClient::HandleGetDataMessage(HandleMessageTask *task){}
    void NodeClient::HandleGetBlocksMessage(HandleMessageTask* msg){}
    void NodeClient::HandleInventoryMessage(HandleMessageTask* msg){}
    void NodeClient::HandleTransactionMessage(HandleMessageTask* msg){}
    void NodeClient::HandleCommitMessage(HandleMessageTask* msg){}
    void NodeClient::HandlePrepareMessage(HandleMessageTask* msg){}
    void NodeClient::HandlePromiseMessage(HandleMessageTask* msg){}
    void NodeClient::HandleRejectedMessage(HandleMessageTask* msg){}
    void NodeClient::HandleAcceptedMessage(HandleMessageTask* msg){}
    void NodeClient::HandleNotFoundMessage(HandleMessageTask* msg){}
    void NodeClient::HandleTestMessage(HandleMessageTask* task){}

    // commands:
    // - ping
    // - gethead
    // - getutxos
    // - getdata
    // - append
    void NodeClient::OnCommandReceived(uv_stream_t *stream, ssize_t nread, const uv_buf_t *buf){
        NodeClient* client = (NodeClient*)stream->data;

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

        std::string line(buf->base, nread - 1);

#ifdef TOKEN_DEBUG
        LOG(INFO) << "parsed: " << line;
#endif//TOKEN_DEBUG

        Command* command = nullptr;
        if(!(command = Command::ParseCommand(line))){
            LOG(WARNING) << "couldn't parse command: " << line;
            return;
        }

#define DECLARE_HANDLER(Name, Text, Parameters) \
    if(command->Is##Name##Command()){ \
        client->Handle##Name##Command(command->As##Name##Command()); \
        delete command; \
        return; \
    }
        FOR_EACH_COMMAND(DECLARE_HANDLER);
#undef DECLARE_HANDLER
        if(command != nullptr){
            LOG(WARNING) << "couldn't handle command: " << command->GetName(); //TODO: handle better
        }
    }

    void NodeClient::HandleStatusCommand(StatusCommand* cmd){
        LOG(INFO) << "Peer: " << GetPeer();
        if(IsDisconnected()){
            LOG(INFO) << "Status: Disconnected.";
        } else if(IsConnecting()){
            LOG(INFO) << "Status: Connecting...";
        } else if(IsConnected()){
            LOG(INFO) << "Status: Connected!";
        }
    }

    void NodeClient::HandleDisconnectCommand(DisconnectCommand* cmd){
        LOG(WARNING) << "disconnecting...";
    }

    void NodeClient::HandleTestCommand(TestCommand* cmd){
        std::string nonce = GenerateNonce();
        LOG(INFO) << "sending nonce: " << nonce;
        Send(TestMessage::NewInstance(HashFromHexString(nonce)));
    }

    //TODO:
    // need to wait
    // in order to wait:
    //   - send_gethead()
    //   - spawn_response_handler_task() ------------> - wait_for(cv_)
    //   - continue                               /        .....
    //      .....                               /          .....
    //   - receive_head                       /            .....
    //   - signal_all() --------------------/          - log_head()
    void NodeClient::HandleTransactionCommand(TransactionCommand* cmd){
        uint256_t tx_hash = cmd->GetNextArgumentHash();
        uint32_t index = cmd->GetNextArgumentUInt32();
        std::string user = cmd->GetNextArgument();

        Transaction::InputList inputs = {
            Input(tx_hash, index, "TestUser"),
        };
        Transaction::OutputList outputs = {
            Output(user, "TestToken")
        };

        Scope scope;
        Transaction* tx = scope.Retain(Transaction::NewInstance(0, inputs, outputs));
        TransactionMessage* msg = scope.Retain(TransactionMessage::NewInstance(tx));
        LOG(INFO) << "sending transaction: " << tx->GetSHA256Hash();
        Send(msg);
    }

    void NodeClient::HandlePingMessage(HandleMessageTask* task){}
    void NodeClient::HandlePongMessage(HandleMessageTask* task){}
}