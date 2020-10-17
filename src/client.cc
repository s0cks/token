#include "server.h"
#include "task.h"
#include "vthread.h"
#include "client.h"
#include "byte_buffer.h"

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

    UUID ClientSessionInfo::GetPeerID() const{
        return ((ClientSession*)GetSession())->GetPeerID();
    }

    void ClientSessionInfo::operator=(const ClientSessionInfo& info){
        SessionInfo::operator=(info);
    }

    ClientSession::ClientSession(const NodeAddress& address):
        Session(&stream_),
        thread_(),
        sigterm_(),
        sigint_(),
        stream_(),
        hb_timer_(),
        hb_timeout_(),
        shutdown_(),
        address_(address),
        peer_id_(),
        head_(){
        shutdown_.data = this;
        stream_.data = this;
        hb_timer_.data = this;
        hb_timeout_.data = this;
    }

    void ClientSession::OnSignal(uv_signal_t* handle, int signum){
        uv_stop(handle->loop);
    }

    void ClientSession::OnShutdown(uv_async_t* handle){
        PeerSession* session = (PeerSession*)handle->data;
        session->Disconnect();
    }

    void ClientSession::OnHeartbeatTick(uv_timer_t* handle){
        ClientSession* client = (ClientSession*)handle->data;

        LOG(INFO) << "sending ping to peer....";
        client->Send(VerackMessage::NewInstance(client->GetID()));
        uv_timer_start(&client->hb_timeout_, &OnHeartbeatTimeout, Session::kHeartbeatTimeoutMilliseconds, 0);
    }

    void ClientSession::OnHeartbeatTimeout(uv_timer_t* handle){
        ClientSession* client = (ClientSession*)handle->data;

        LOG(WARNING) << "peer " << client->GetID() << " timed out, disconnecting....";
        //TODO: disconnect client
    }

    void* ClientSession::ClientSessionThread(void* data){
        ClientSession* session = (ClientSession*)data;
        NodeAddress address = session->GetPeerAddress();
        LOG(INFO) << "connecting to peer " << address << "....";

        uv_loop_t* loop = uv_loop_new();
        uv_loop_init(loop);
        uv_timer_init(loop, &session->hb_timer_);
        uv_timer_init(loop, &session->hb_timeout_);
        uv_signal_init(loop, &session->sigterm_);
        uv_signal_start(&session->sigterm_, &OnSignal, SIGTERM);

        uv_signal_init(loop, &session->sigint_);
        uv_signal_start(&session->sigterm_, &OnSignal, SIGINT);

        uv_async_init(loop, &session->shutdown_, &OnShutdown);

        uv_tcp_init(loop, &session->stream_);

        struct sockaddr_in addr;
        uv_ip4_addr(address.GetAddress().c_str(), address.GetPort(), &addr);

        uv_connect_t request;
        request.data = session;

        int err;
        if((err = uv_tcp_connect(&request, &session->stream_, (const struct sockaddr*)&addr, &OnConnect)) != 0){
            LOG(WARNING) << "couldn't connect to peer: " << uv_strerror(err);
            goto cleanup;
        }

        uv_run(loop, UV_RUN_DEFAULT);
        uv_signal_stop(&session->sigterm_);
        uv_signal_stop(&session->sigint_);
    cleanup:
        uv_loop_close(loop);
        LOG(INFO) << "disconnected from peer: " << address;
        pthread_exit(0);
    }

    bool ClientSession::Connect(){
        int ret;
        if((ret = pthread_create(&thread_, NULL, &ClientSessionThread, this)) != 0){
            LOG(ERROR) << "couldn't start client thread: " << strerror(ret);
            return false;
        }
        return true;
    }

    void ClientSession::OnConnect(uv_connect_t* conn, int status){
        ClientSession* session = (ClientSession*)conn->data;
        if(status != 0){
            LOG(WARNING) << "client accept error: " << uv_strerror(status);
            session->Disconnect();
            return;
        }

        session->SetState(Session::kConnecting);
        if((status = uv_read_start(session->GetStream(), &AllocBuffer, &OnMessageReceived)) != 0){
            LOG(WARNING) << "client read error: " << uv_strerror(status);
            session->Disconnect();
            return;
        }

        if((status = uv_timer_start(&session->hb_timer_, &OnHeartbeatTick, 0, Session::kHeartbeatIntervalMilliseconds)) != 0){
            LOG(WARNING) << "couldn't start the heartbeat timer: " << uv_strerror(status);
            return; //TODO: terminate connection
        }

        session->Send(VersionMessage::NewInstance(session->GetID()));
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
        ByteBuffer bytes((uint8_t*)buff->base, buff->len);
        do{
            uint32_t mtype = bytes.GetInt();
            intptr_t msize = bytes.GetLong();

            switch(mtype) {
#define DEFINE_DECODE(Name) \
                case Message::MessageType::k##Name##MessageType:{ \
                    Handle<Message> msg = Name##Message::NewInstance(&bytes).CastTo<Message>(); \
                    LOG(INFO) << "decoded: " << msg << " (" << msize << " bytes)"; \
                    messages.push_back(msg); \
                    break; \
                }
                FOR_EACH_MESSAGE_TYPE(DEFINE_DECODE)
#undef DEFINE_DECODE
                case Message::MessageType::kUnknownMessageType:
                default:
                    LOG(ERROR) << "unknown message type " << mtype << " of size " << msize;
                    break;
            }

            offset += (msize + Message::kHeaderSize);
        } while((offset + Message::kHeaderSize) < nread);

        for(size_t idx = 0; idx < messages.size(); idx++){
            Handle<Message> msg = session->next_ = messages[idx];
            session->OnNextMessageReceived(msg);
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
        Handle<BlockMessage> msg = task->GetMessage().CastTo<BlockMessage>();

        Block* blk = msg->GetBlock();
        LOG(INFO) << "received block: " << blk->GetHeader();
    }

    void ClientSession::HandleInventoryMessage(const Handle<HandleMessageTask>& task){
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
        if(pthread_self() == thread_){
            // Inside Session OSThreadBase
            uv_read_stop((uv_stream_t*)&stream_);
            uv_stop(stream_.loop);
            uv_loop_close(stream_.loop);
            SetState(State::kDisconnected);
        } else{
            // Outside Session OSThreadBase
            uv_async_send(&shutdown_);
        }
    }

    bool BlockChainClient::Connect(){
        if(!GetSession()->Connect())
            return false;
        // this will cause an indefinite wait if the connection fails before the state hits kConnected
        GetSession()->WaitForState(Session::kConnected);
        return true;
    }

    bool BlockChainClient::Disconnect(){
        GetSession()->Disconnect();
        return true;//TODO: better response for BlockChainClient::Disconnect()
    }

    bool BlockChainClient::Send(const Handle<Transaction>& tx){
        LOG(INFO) << "sending transaction " << tx->GetHash();
        ClientSession* session = GetSession();
        if(session->IsConnecting()){
            LOG(INFO) << "waiting for client to connect...";
            session->WaitForState(Session::kConnected);
        }
        session->Send(TransactionMessage::NewInstance(tx));
        return true; // no acknowledgement from server
    }

    bool BlockChainClient::GetUnclaimedTransactions(const User& user, std::vector<Hash>& utxos){
        LOG(INFO) << "getting unclaimed transactions for " << user;
        ClientSession* session = GetSession();
        if(session->IsConnecting()){
            LOG(INFO) << "waiting for client to connect...";
            session->WaitForState(Session::kConnected);
        }

        session->Send(GetUnclaimedTransactionsMessage::NewInstance(user));
        do{
            session->WaitForNextMessage();
            Handle<Message> next = session->GetNextMessage();
            if(next->IsInventoryMessage()){
                Handle<InventoryMessage> inv = next.CastTo<InventoryMessage>();

                std::vector<InventoryItem> items;
                if(!inv->GetItems(items)){
                    LOG(ERROR) << "cannot get items from inventory";
                    return false;
                }

                LOG(INFO) << "received inventory of " << items.size() << " items";
                for(auto& item : items)
                    utxos.push_back(item.GetHash());
                return utxos.size() == items.size();
            } else{
                LOG(WARNING) << "cannot handle " << next;
                continue;
            }
        } while(true);
    }
}