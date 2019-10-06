#include <algorithm>
#include <glog/logging.h>
#include <sys/time.h>

#include "node/peer.h"
#include "blockchain.h"
#include "node/server.h"

namespace Token{
    static uint32_t
    GetCurrentTime(){
        struct timeval time;
        gettimeofday(&time, NULL);
        uint32_t curr_time = ((uint32_t)time.tv_sec * 1000 + time.tv_usec / 1000);
        return curr_time;
    }

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

    void PeerClient::DownloadBlock(const std::string &hash){
        LOG(INFO) << "downloading block: " << hash;
        /*
        TODO:
        Node::Messages::GetBlockRequest getblock;
        getblock.set_hash(hash);
        Message msg(Message::Type::kGetBlockMessage, &getblock);
        Send(&msg);
        */
    }

    void PeerClient::OnConnect(uv_connect_t *conn, int status) {
        if (status == 0) {
            uv_async_init(loop_, &async_send_, OnAsyncSend);
            BlockChainServer::Register(this);
            state_ = State::kConnecting;
            handle_ = conn->handle;
            handle_->data = conn->data;
            SendVersionMessage();
            uv_read_start(handle_, AllocBuffer, [](uv_stream_t *stream, ssize_t nread, const uv_buf_t *buff) {
                PeerClient *peer = (PeerClient*)stream->data;
                peer->OnMessageRead(stream, nread, buff);
            });
        }
    }

    bool PeerClient::Handle(uv_stream_t* stream, Token::Message* msg) {
        if(msg->IsBlockMessage()){
            Token::Block* block = Token::Block::Decode(msg->GetAsBlock());
            if(!BlockChain::ContainsBlock(block->GetHash())){
                LOG(INFO) << "received block: " << block->GetHash();
                if(!BlockChain::AppendBlock(block)){
                    LOG(ERROR) << "couldn't append new block: " << block->GetHash();
                    return false;
                }
            }
            Message m(Message::Type::kBlockMessage, block->GetAsMessage());
            BlockChainServer::GetInstance()->AsyncBroadcast(&m);
            return true;
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

    void PeerClient::SendVersionMessage(){
        //TODO: Implement
    }
}