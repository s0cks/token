#include "server.h"
#include "task.h"
#include "client.h"
#include "bytes.h"

namespace Token{
    ClientSessionInfo::ClientSessionInfo(ClientSession* session):
        SessionInfo(session){
    }

    BlockHeader ClientSessionInfo::GetHead() const{
        return ((ClientSession*)GetSession())->GetHead();
    }

    NodeAddress ClientSessionInfo::GetPeerAddress() const{
        return ((ClientSession*)GetSession())->GetPeerAddress();
    }

    std::string ClientSessionInfo::GetPeerID() const{
        return ((ClientSession*)GetSession())->GetPeerID();
    }

    void ClientSessionInfo::operator=(const ClientSessionInfo& info){
        SessionInfo::operator=(info);
    }

    void ClientSession::OnSignal(uv_signal_t* handle, int signum){
        ClientSession* client = (ClientSession*)handle->data;
        uv_stop(client->loop_);
    }

    void ClientSession::OnHeartbeatTick(uv_timer_t* handle){
        ClientSession* client = (ClientSession*)handle->data;

        LOG(INFO) << "sending ping to peer....";
        client->Send(VersionMessage::NewInstance(client->GetID()));
        uv_timer_start(&client->hb_timeout_, &OnHeartbeatTimeout, Session::kHeartbeatTimeoutMilliseconds, 0);
    }

    void ClientSession::OnHeartbeatTimeout(uv_timer_t* handle){
        ClientSession* client = (ClientSession*)handle->data;

        LOG(WARNING) << "peer " << client->GetID() << " timed out, disconnecting....";
        //TODO: disconnect client
    }

    void ClientSession::Connect(const NodeAddress& addr){
        LOG(INFO) << "connecting to server: " << addr;
        SetState(State::kConnecting);

        uv_loop_init(loop_);
        uv_timer_init(loop_, &hb_timer_);
        uv_timer_init(loop_, &hb_timeout_);
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

    void ClientSession::OnConnect(uv_connect_t* conn, int status){
        if(status != 0){
            LOG(WARNING) << "error connecting to server: " << uv_strerror(status);
            return; //TODO: terminate connection
        }

        ClientSession* client = (ClientSession*)conn->data;

        int err;
        if((err = uv_read_start(client->GetStream(), &AllocBuffer, &OnMessageReceived)) != 0){
            LOG(ERROR) << " client read error: " << uv_strerror(err);
            return; //TODO: terminate connection
        }

        if((err = uv_read_start(client->GetStdinPipe(), &AllocBuffer, &OnCommandReceived)) != 0){
            LOG(ERROR) << "couldn't start reading from stdin-pipe: " << uv_strerror(err);
            return; //TODO: terminate connection
        }

        if((err = uv_timer_start(&client->hb_timer_, &OnHeartbeatTick, 0, Session::kHeartbeatIntervalMilliseconds)) != 0){
            LOG(WARNING) << "couldn't start the heartbeat timer: " << uv_strerror(err);
            return; //TODO: terminate connection
        }

        client->Send(VersionMessage::NewInstance(client->GetID()));
    }

    void ClientSession::OnMessageReceived(uv_stream_t* stream, ssize_t nread, const uv_buf_t* buff){
        ClientSession* session = (ClientSession*)stream->data;
        if(nread == UV_EOF){
            LOG(ERROR) << "client disconnected!";
            return;
        } else if(nread < 0){
            LOG(ERROR) << "[" << nread << "] client read error: " << std::string(uv_strerror(nread));
            return;
        } else if(nread == 0){
            LOG(WARNING) << "zero message size received";
            return;
        } else if(nread >= Session::kBufferSize){
            LOG(ERROR) << "too large of a buffer";
            return;
        }

        uint32_t offset = 0;
        std::vector<Message*> messages;
        do{
            uint32_t mtype = 0;
            memcpy(&mtype, &buff->base[offset + Message::kTypeOffset], Message::kTypeLength);

            uint64_t msize = 0;
            memcpy(&msize, &buff->base[offset + Message::kSizeOffset], Message::kSizeLength);

            ByteBuffer bytes((uint8_t*)&buff->base[offset + Message::kDataOffset], msize);

            Handle<Message> msg = Message::Decode(static_cast<Message::MessageType>(mtype), &bytes);
            LOG(INFO) << "decoded message: " << msg->ToString(); //TODO: handle decode failures
            messages.push_back(msg);

            offset += (msize + Message::kHeaderSize);
        } while((offset + Message::kHeaderSize) < nread);

        for(size_t idx = 0; idx < messages.size(); idx++){
            Handle<Message> msg = messages[idx];
            Handle<HandleMessageTask> task = HandleMessageTask::NewInstance(session, msg);
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
        }
    }

    void ClientSession::HandleVersionMessage(const Handle<HandleMessageTask>& task){
        ClientSession* client = (ClientSession*)task->GetSession();
        Handle<VersionMessage> msg = task->GetMessage().CastTo<VersionMessage>();
        client->Send(VerackMessage::NewInstance(client->GetID()));
    }

    void ClientSession::HandleVerackMessage(const Handle<HandleMessageTask>& task){
        ClientSession* client = (ClientSession*)task->GetSession();
        Handle<VerackMessage> msg = task->GetMessage().CastTo<VerackMessage>();
        if(IsConnecting()){
            LOG(INFO) << "client connected!";
            client->SetState(State::kConnected);
        }
        client->SetHead(msg->GetHead());
        client->SetPeerID(msg->GetID());
        client->SetPeerAddress(msg->GetCallbackAddress());
        if(IsConnected()){
            uv_timer_stop(&client->hb_timeout_);
        }
    }

    void ClientSession::HandleBlockMessage(const Handle<HandleMessageTask>& task){
        ClientSession* client = (ClientSession*)task->GetSession();
        Handle<BlockMessage> msg = task->GetMessage().CastTo<BlockMessage>();

        Block* blk = msg->GetBlock();
        LOG(INFO) << "received block: " << blk->GetHeader();
    }

    void ClientSession::HandleInventoryMessage(const Handle<HandleMessageTask>& task){
        ClientSession* session = (ClientSession*)task->GetSession();
        Handle<InventoryMessage> msg = task->GetMessage().CastTo<InventoryMessage>();

        std::vector<InventoryItem> items;
        if(!msg->GetItems(items)){
            LOG(WARNING) << "couldn't read inventory from peer";
            return;
        }

        for(auto& item : items){
            LOG(INFO) << "  - " << item.GetHash();
        }
    }

    void ClientSession::HandleGetDataMessage(const Handle<HandleMessageTask>& task){}
    void ClientSession::HandleGetBlocksMessage(const Handle<HandleMessageTask>& msg){}
    void ClientSession::HandleTransactionMessage(const Handle<HandleMessageTask>& msg){}
    void ClientSession::HandleCommitMessage(const Handle<HandleMessageTask>& msg){}
    void ClientSession::HandlePrepareMessage(const Handle<HandleMessageTask>& msg){}
    void ClientSession::HandlePromiseMessage(const Handle<HandleMessageTask>& msg){}
    void ClientSession::HandleRejectedMessage(const Handle<HandleMessageTask>& msg){}
    void ClientSession::HandleAcceptedMessage(const Handle<HandleMessageTask>& msg){}
    void ClientSession::HandleNotFoundMessage(const Handle<HandleMessageTask>& msg){}
    void ClientSession::HandleGetUnclaimedTransactionsMessage(const Handle<HandleMessageTask>& task){}

    void ClientSession::OnCommandReceived(uv_stream_t *stream, ssize_t nread, const uv_buf_t *buf){
        ClientSession* client = (ClientSession*)stream->data;

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

    void ClientSession::HandleStatusCommand(StatusCommand* cmd){
        ClientSessionInfo info = GetInfo();
        LOG(INFO) << "Client ID: " << info.GetID();
        switch(info.GetState()){
            case Session::State::kDisconnected:
                LOG(INFO) << "Disconnected!";
                break;
            case Session::State::kConnecting:
                LOG(INFO) << "Connecting....";
                break;
            case Session::State::kConnected:
                LOG(INFO) << "Connected.";
                break;
            default:
                LOG(WARNING) << "Unknown Connection Status!";
                break;
        }
        LOG(INFO) << "Peer Information:";
        LOG(INFO) << "  - ID: " << info.GetPeerID();
        LOG(INFO) << "  - Address: " << info.GetPeerAddress();
        LOG(INFO) << "  - Head: " << info.GetHead();

    }

    void ClientSession::HandleDisconnectCommand(DisconnectCommand* cmd){
        LOG(WARNING) << "disconnecting...";
        uv_read_stop((uv_stream_t*)&connection_);
        uv_stop(loop_);
        uv_loop_close(loop_);
        SetState(State::kDisconnected);
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
    void ClientSession::HandleTransactionCommand(TransactionCommand* cmd){
        uint256_t tx_hash = cmd->GetNextArgumentHash();
        uint32_t index = cmd->GetNextArgumentUInt32();
        std::string user = cmd->GetNextArgument();

        Input* inputs[1] = {
            Input::NewInstance(tx_hash, index, "TestUser")
        };

        Output* outputs[1] = {
            Output::NewInstance(user, "TestToken")
        };

        Handle<Transaction> tx = Transaction::NewInstance(0, inputs, 1, outputs, 1);
        Handle<TransactionMessage> msg = TransactionMessage::NewInstance(tx);
        LOG(INFO) << "sending transaction: " << tx->GetHash();
        Send(msg);
    }

    void ClientSession::HandleGetTokensCommand(Token::GetTokensCommand* cmd){
        std::string user = cmd->GetNextArgument();

        LOG(INFO) << "getting tokens for " << user << "....";
        Send(GetUnclaimedTransactionsMessage::NewInstance(user));
    }
}