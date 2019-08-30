#include <glog/logging.h>
#include "peer.h"
#include "blockchain.h"
#include "server.h"

namespace Token{
    static void
    AllocBuffer(uv_handle_t *handle, size_t suggested_size, uv_buf_t *buff) {
        char* base = (char*)malloc(suggested_size);
        *buff = uv_buf_init(base, suggested_size);
    }

    struct SendCommand{
        PeerClient* peer;
        Message* message;
    };

    static void OnAsyncSend(uv_async_t* handle){
        SendCommand* cmd = (SendCommand*) handle->data;
        LOG(INFO) << "AsyncSend";
        cmd->peer->Send(cmd->message);
        free(cmd);
    }

    PeerClient::PeerClient(const std::string &addr, int port):
        loop_(uv_loop_new()),
        address_(addr),
        port_(port),
        stream_(),
        connection_(),
        state_(State::kConnecting),
        async_send_(),
        handle_(nullptr){}

    PeerClient::~PeerClient(){}

    bool PeerClient::AcceptsIdentity(Token::Node::Messages::PeerIdentity* pident){
        return true;
    }

    /*
    uint32_t PeerClient::GetTime(){
        struct timeval time;
        gettimeofday(&time, NULL);
        uint32_t curr_time = ((uint32_t)time.tv_sec * 1000 + time.tv_usec / 1000);
        return (last_time_ = curr_time);
    }
     */

    void PeerClient::DownloadBlock(const std::string &hash){
        LOG(INFO) << "downloading block: " << hash;
        Node::Messages::GetBlockRequest getblock;
        getblock.set_hash(hash);
        Message msg(Message::Type::kGetBlockMessage, &getblock);
        Send(&msg);
    }

    void PeerClient::SendIdentity(){
        Node::Messages::PeerIdentity ident;
        ident.set_version("1.0.0");
        if(BlockChain::GetInstance()->HasHead()){
            ident.set_head(BlockChain::GetInstance()->GetHead()->GetHash());
        }
        // Set peers
        std::vector<PeerClient*> peers;
        if(!BlockChainServer::GetPeerList(peers)){
            LOG(ERROR) << "couldn't get peer list";
            return;
        }

        for(auto& it : peers){
            ident.add_peers()->set_address(it->ToString());
        }

        // Set blocks
        std::vector<std::string> blocks;
        if(!BlockChain::GetBlockList(blocks)){
            LOG(ERROR) << "couldn't get block list";
            return;
        }

        for(auto& it : blocks){
            ident.add_blocks(it);
        }

        Message msg(Message::Type::kPeerIdentityMessage, &ident);
        Send(&msg);
    }

    void PeerClient::OnConnect(uv_connect_t *conn, int status) {
        if (status == 0) {
            uv_async_init(loop_, &async_send_, OnAsyncSend);
            BlockChainServer::Register(this);
            state_ = State::kConnecting;
            handle_ = conn->handle;
            handle_->data = conn->data;
            SendIdentity();
            uv_read_start(handle_, AllocBuffer, [](uv_stream_t *stream, ssize_t nread, const uv_buf_t *buff) {
                PeerClient *peer = (PeerClient*)stream->data;
                peer->OnMessageRead(stream, nread, buff);
            });
        }
    }

    bool PeerClient::Handle(uv_stream_t* stream, Token::Message* msg) {
        LOG(INFO) << "handling: " << msg->ToString() << "...";
        if(msg->IsBlockMessage()){
            Token::Block* block = Token::Block::Load(msg->GetAsBlock());
            if(IsSynchronizing()){
                LOG(WARNING) << "downloaded block: " << block->GetHash();
                LOG(WARNING) << (*block);
                BlockChain::Save(block);
                return true;
            } else if(IsConnected()){
                LOG(INFO) << "received block: " << block->GetHash();
                if(!BlockChain::GetInstance()->Append(block)){
                    LOG(ERROR) << "couldn't append block: " << block->GetHash();
                    return false;
                }
                Message m(Message::Type::kBlockMessage, block->GetAsMessage());
                BlockChainServer::GetInstance()->AsyncBroadcast(&m);
                return true;
            }
            LOG(ERROR) << "invalid state to receive block";
            return false;
        } else if(msg->IsPeerIdentityMessage()){
            Node::Messages::PeerIdentity* ident = msg->GetAsPeerIdentity();
            //TODO: Connect to peers
            if(IsConnecting()){
                SetState(State::kSynchronizing);
                for(auto& it : ident->blocks()){
                    if(!BlockChain::GetInstance()->HasBlock(it)){
                        DownloadBlock(it);
                        sleep(5);
                    }
                }
                SendIdentity();
                return true;
            } else if(IsSynchronizing()){
                bool synced = true;
                for(auto& it : ident->blocks()){
                    if(!BlockChain::GetInstance()->HasBlock(it)){
                        synced = false;
                        DownloadBlock(it);
                    }
                }

                if(!synced){
                    LOG(INFO) << "sending identity";
                    SendIdentity();
                    return true;
                }

                if(!BlockChain::GetInstance()->SetHead(ident->head())){
                    LOG(ERROR) << "couldn't set head";
                    return false;
                }

                LOG(INFO) << "connected!";
                SetState(State::kConnected);
                return true;
            }
        }
    }

    void PeerClient::OnMessageRead(uv_stream_t *stream, ssize_t nread, const uv_buf_t *buff) {
        PeerClient *peer = (PeerClient*)stream->data;
        if (nread == UV_EOF) {
            LOG(ERROR) << "disconnected from peer";
            uv_read_stop(stream);
            return;
        } else if (nread > 0) {
            Message msg;
            if(!msg.Decode((uint8_t*)buff->base, nread)){
                LOG(ERROR) << "couldn't decode message";
                uv_read_stop(stream);
                return;
            }
            if(!peer->Handle(stream, &msg)){
                LOG(ERROR) << "couldn't handle message: " << msg.ToString();
                uv_read_stop(stream);
                return;
            }
        }
    }

    int PeerClient::ConnectToPeer(){
        LOG(INFO) << "connecting to peer " << GetAddress() << ":" << GetPort() << "..." << std::endl;
        loop_ = uv_loop_new();
        uv_tcp_init(loop_, &stream_);
        sockaddr_in addr;
        uv_ip4_addr(address_.c_str(), port_, &addr);
        connection_.data = (void*)this;
        int err = uv_tcp_connect(&connection_, &stream_, (const struct sockaddr *)&addr, [](uv_connect_t *conn, int status) {
            PeerClient *peer = (PeerClient *)conn->data;
            peer->OnConnect(conn, status);
        });
        if(err != 0){
            LOG(ERROR) << "cannot connect to peer: " << std::string(uv_strerror(err));
            return err;
        }
        uv_run(loop_, UV_RUN_DEFAULT);
        return err;
    }

    void* PeerClient::PeerSessionThread(void* data){
        PeerClient* instance = (PeerClient*)data;

        char* result;
        int rc;
        if((rc = instance->ConnectToPeer()) < 0){
            const char* err = uv_strerror(rc);
            result = (char*)malloc(sizeof(err));
            strcpy(result, err);
        } else{
            result = (char*)"done";
        }
        pthread_exit(result);
    }

    bool PeerClient::Connect(){
        if(!IsConnecting()) return false;
        pthread_create(&thread_, NULL, &PeerSessionThread, this);
        return true;
    }

    bool PeerClient::Disconnect(){
        void* result;
        if(pthread_join(thread_, &result) != 0){
            LOG(ERROR) << "unable to join peer thread";
            return false;
        }
        LOG(WARNING) << "peer disconnected with: " << (char*)result;
        return true;
    }

    void
    PeerClient::AsyncSend(Token::Message *msg){
        SendCommand cmd = {
                .peer = this,
                .message = new Token::Message(*msg),
        };
        async_send_.data = malloc(sizeof(SendCommand));
        memcpy(async_send_.data, &cmd, sizeof(SendCommand));
        uv_async_send(&async_send_);
    }

    bool
    PeerClient::Send(Token::Message *msg) {
        size_t size = msg->GetMessageSize() + (sizeof(uint32_t) * 2);

        LOG(WARNING) << "sending message of size: " << size;
        uint8_t bytes[size];
        msg->Encode(bytes, size);

        uv_buf_t buff = uv_buf_init((char*)bytes, size);
        uv_write_t *req = (uv_write_t *) malloc(sizeof(uv_write_t));
        req->data = (void *) this;
        uv_write(req, handle_, &buff, 1, [](uv_write_t *req, int status){
            PeerClient* peer = (PeerClient*)req->data;
            peer->OnMessageSent(req, status);
        });
        return true;
    }

    void PeerClient::OnMessageSent(uv_write_t *req, int status) {
        if (status != 0) {
            std::cerr << "Error writing message" << std::endl;
            return;
        }
        if (req) free(req);
    }

    std::string PeerClient::ToString() const{
        std::stringstream stream;
        stream << GetAddress() << ":" << GetPort();
        return stream.str();
    }
}