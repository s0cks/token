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

    ClientSession::ClientSession(bool use_stdin):
        stream_(),
        sigterm_(),
        sigint_(),
        handler_(nullptr),
        Session(&stream_){
        stream_.data = this;
        connection_.data = this;
        hb_timer_.data = this;
        hb_timeout_.data = this;
        if(use_stdin){
            handler_ = new ClientCommandHandler(this);
        }
    }

    void ClientSession::OnSignal(uv_signal_t* handle, int signum){
        ClientSession* client = (ClientSession*)handle->data;
        uv_stop(handle->loop);
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

        uv_loop_t* loop = uv_loop_new();
        uv_loop_init(loop);
        uv_timer_init(loop, &hb_timer_);
        uv_timer_init(loop, &hb_timeout_);
        uv_signal_init(loop, &sigterm_);
        uv_signal_start(&sigterm_, &OnSignal, SIGTERM);

        uv_signal_init(loop, &sigint_);
        uv_signal_start(&sigterm_, &OnSignal, SIGINT);

        uv_tcp_init(loop, &stream_);
        uv_tcp_keepalive(&stream_, 1, 60);

        struct sockaddr_in address;
        if(!addr.Get(&address)){
            LOG(WARNING) << "couldn't resolved address '" << addr;
            goto cleanup;
        }

        int err;
        if((err = uv_tcp_connect(&connection_, &stream_, (const struct sockaddr*)&address, &OnConnect)) != 0){
            LOG(WARNING) << "couldn't connect to peer: " << uv_strerror(err);
            goto cleanup;
        }

        if(handler_ != nullptr){
            handler_->Start();
        }

        uv_run(loop, UV_RUN_DEFAULT);
        uv_signal_stop(&sigterm_);
        uv_signal_stop(&sigint_);
    cleanup:
        uv_loop_close(loop);
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

        if((err = uv_timer_start(&client->hb_timer_, &OnHeartbeatTick, 0, Session::kHeartbeatIntervalMilliseconds)) != 0){
            LOG(WARNING) << "couldn't start the heartbeat timer: " << uv_strerror(err);
            return; //TODO: terminate connection
        }

        client->Send(VersionMessage::NewInstance(client->GetID()));
    }

    void ClientSession::OnMessageReceived(uv_stream_t* stream, ssize_t nread, const uv_buf_t* buff){
        ClientSession* session = (ClientSession*)stream->data;
        if(nread == UV_EOF){
            LOG(ERROR) << "client disconnected!: " << std::string(uv_strerror(nread));
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
        std::vector<Handle<Message>> messages;
        do{
            ByteBuffer bytes((uint8_t*)buff->base, buff->len);
            uint32_t mtype = bytes.GetInt();
            uint64_t msize = bytes.GetLong();
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

    void ClientSession::Disconnect(){
        LOG(WARNING) << "disconnecting...";
        uv_read_stop(GetStream());
        uv_stop(GetLoop());
        uv_loop_close(GetLoop());
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

    void AllocBuffer(uv_handle_t* handle, size_t suggested_size, uv_buf_t* buff){
        //TODO: buff->base = (char*)Allocator::Allocate(kBufferSize);
        buff->base = (char*)malloc(2048);
        buff->len = 2048;
    }

    void ClientCommandHandler::Start(){
        ClientSession* client = GetSession();
        uv_loop_t* loop = client->GetLoop();

        uv_pipe_init(loop, &stdin_, 0);
        uv_pipe_open(&stdin_, 0);

        int err;
        if((err = uv_read_start((uv_stream_t*)&stdin_, &AllocBuffer, &OnCommandReceived)) != 0){
            LOG(ERROR) << "couldn't start reading from stdin-pipe: " << uv_strerror(err);
            return; //TODO: terminate connection
        }
    }

    void ClientCommandHandler::HandleStatusCommand(ClientCommand* cmd){
        ClientSessionInfo info = GetSession()->GetInfo();
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

    void ClientCommandHandler::HandleDisconnectCommand(ClientCommand* cmd){
        uv_read_stop((uv_stream_t*)&stdin_);
        GetSession()->Disconnect();
    }

    void ClientCommandHandler::HandleTransactionCommand(ClientCommand* cmd){
        uint256_t tx_hash = cmd->GetNextArgumentHash();
        uint32_t index = cmd->GetNextArgumentUnsignedInt();
        std::string user = cmd->GetNextArgument();

        Input* inputs[] = {
            Input::NewInstance(tx_hash, index, "TestUser"),
        };

        Output* outputs[] = {
            Output::NewInstance(user, "TestToken"),
        };

        Handle<Transaction> tx = Transaction::NewInstance(0, inputs, 1, outputs, 1);
        Handle<TransactionMessage> msg = TransactionMessage::NewInstance(tx);
        LOG(INFO) << "sending transaction: " << tx->GetHash();
        Send(msg);
    }

    void ClientCommandHandler::OnCommandReceived(uv_stream_t *stream, ssize_t nread, const uv_buf_t* buff){
        ClientCommandHandler* handler = (ClientCommandHandler*)stream->data;
        ClientSession* client = handler->GetSession();

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

        std::string line(buff->base, nread-1);
        std::deque<std::string> args;
        SplitString(line, args, ' ');

        ClientCommand cmd(args);
#define DECLARE_CHECK(Name, Text, ArgumentCount) \
            if(cmd.Is##Name##Command()){ \
                handler->Handle##Name##Command(&cmd); \
                return; \
            }
        FOR_EACH_CLIENT_COMMAND(DECLARE_CHECK);
#undef DECLARE_CHECK
    }
}