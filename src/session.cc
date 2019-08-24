#include <glog/logging.h>
#include "session.h"
#include "blockchain.h"
#include "server.h"

namespace Token {
#define DEFINE_SESSION_TYPECHECK(Name) \
    Name##Session* Name##Session::As##Name##Session(){ return this; }
    FOR_EACH_SESSION(DEFINE_SESSION_TYPECHECK)
#undef DEFINE_SESSION_TYPECHECK

    static void
    AllocBuffer(uv_handle_t *handle, size_t suggested_size, uv_buf_t *buff) {
        char* base = (char*)malloc(suggested_size);
        *buff = uv_buf_init(base, suggested_size);
    }

    void
    Session::OnMessageSent(uv_write_t *req, int status) {
        if (status != 0) {
            std::cerr << "Error writing message" << std::endl;
            return;
        }
        if (req) free(req);
    }

    bool
    Session::Send(uv_stream_t* stream, Token::Message *msg) {
        size_t size = msg->GetMessageSize() + (sizeof(uint32_t) * 2);

        LOG(WARNING) << "sending message of size: " << size;
        uint8_t bytes[size];
        msg->Encode(bytes, size);

        uv_buf_t buff = uv_buf_init((char*)bytes, size);
        uv_write_t *req = (uv_write_t *) malloc(sizeof(uv_write_t));
        req->data = (void *) this;
        uv_write(req, GetStream(), &buff, 1, [](uv_write_t *req, int status) {
            Session *session = (Session *)req->data;
            session->OnMessageSent(req, status);
        });
        return true;
    }

    static void*
    PeerSessionThread(void* data){
        char* result = (char*)malloc(sizeof(char) * 128);
        PeerSession* peer = (PeerSession*)data;
        int rc;
        if((rc = peer->ConnectToPeer()) < 0){
            strcpy(result, uv_strerror(rc));
        }
        pthread_exit(result);
    }

    int PeerSession::ConnectToPeer(){
        LOG(INFO) << "connecting to peer " << GetAddress() << ":" << GetPort() << "..." << std::endl;
        loop_ = uv_loop_new();
        uv_tcp_init(loop_, &stream_);
        uv_ip4_addr(addressstr_.c_str(), port_, &address_);
        connection_.data = (void*)this;
        int err = uv_tcp_connect(&connection_, &stream_, (const struct sockaddr *) &address_, [](uv_connect_t *conn, int status) {
            PeerSession *peer = (PeerSession *) conn->data;
            peer->OnConnect(conn, status);
        });
        if(err != 0){
            LOG(ERROR) << "cannot connect to peer: " << std::string(uv_strerror(err));
            return err;
        }
        uv_run(loop_, UV_RUN_DEFAULT);
        return err;
    }

    bool PeerSession::Connect() {
        if(IsConnected()) return false;
        pthread_create(&thread_, NULL, &PeerSessionThread, this);
        return true;
    }

    void PeerSession::Disconnect(){
        void* result;
        if(pthread_join(thread_, &result) != 0){
            LOG(ERROR) << "unable to join peer thread";
            return;
        }
        printf("Peer disconnected with: %s\n", result);
    }

    void PeerSession::OnConnect(uv_connect_t *conn, int status) {
        if (status == 0) {
            LOG(INFO) << "connected to peer!";
            LOG(INFO) << "doing handshake...";
            state_ = State::kHandshake;
            handle_ = conn->handle;
            handle_->data = conn->data;
            Message msg(Message::Type::kGetIdentMessage);
            if(!Send(handle_, &msg)){
                LOG(ERROR) << "cannot send message: " << msg.ToString();
                state_ = State::kDisconnected;
                Disconnect();
                return;
            }
            uv_read_start(handle_, AllocBuffer, [](uv_stream_t *stream, ssize_t nread, const uv_buf_t *buff) {
                PeerSession *peer = (PeerSession*) stream->data;
                peer->OnMessageRead(stream, nread, buff);
            });
        }
    }

    void PeerSession::OnMessageRead(uv_stream_t *stream, ssize_t nread, const uv_buf_t *buff) {
        PeerSession *peer = (PeerSession *) stream->data;
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
            peer->ClearReadBuffer();
            if(!peer->Handle(stream, &msg)){
                LOG(ERROR) << "couldn't handle message";
                uv_read_stop(stream);
                return;
            }
        }
    }

    bool PeerSession::Handle(uv_stream_t* stream, Token::Message* msg) {
        if(msg->GetType() == Message::Type::kBlockMessage){
            Token::Block* block = Token::Block::Load(msg->GetAsBlockMessage());
            LOG(INFO) << "received block: " << block->GetHash();
            return true;
        } else if(msg->GetType() == Message::Type::kGetIdentMessage){
            if(GetState() != State::kHandshake){
                LOG(ERROR) << "peer is not in handshake state";
                return false;
            }
            Messages::PeerIdentity* ident = msg->GetAsIdentMessage();
            Messages::BlockHeader phead = ident->head();
            if(!BlockChain::GetInstance()->GetHead()->Equals(phead)){
                LOG(ERROR) << "incompatible <HEAD>";
                return false;
            }
            state_ = State::kConnected;
            LOG(INFO) << "peer connected";
            return true;
        }
    }

    PeerSession::PeerSession(BlockChainServer* parent, std::string addr, int port) :
            loop_(nullptr),
            addressstr_(addr),
            port_(port),
            stream_(),
            connection_(),
            handle_(nullptr),
            address_(),
            thread_(),
            Session(parent){

    }

    bool ClientSession::Handle(uv_stream_t* stream, Token::Message *msg){
        if(msg->GetType() == Message::Type::kBlockMessage){
            Token::Block* block = Token::Block::Load(msg->GetAsBlockMessage());
            LOG(INFO) << "received block: " << block->GetHash();
            if(!BlockChain::GetInstance()->Append(block)){
                LOG(ERROR) << "couldn't append block: " << block->GetHash();
                return false;
            }
            Message m(Message::Type::kBlockMessage, block->GetAsMessage());
            BlockChainServer::Broadcast(stream, &m);
        } else if(msg->GetType() == Message::Type::kGetHeadMessage){
            Token::Block* block = BlockChain::GetInstance()->GetHead();
            Token::Messages::BlockHeader* blk = new Token::Messages::BlockHeader();
            LOG(INFO) << "sending head: " << block->GetHash();
            Message response(Message::Type::kBlockMessage, block->GetAsMessage());
            Send(stream, &response);
        } else if(msg->GetType() == Message::Type::kGetIdentMessage){
            LOG(INFO) << "client requested identity, sending...";
            Token::Messages::PeerIdentity* ident = new Token::Messages::PeerIdentity();
            Block* head = BlockChain::GetInstance()->GetHead();
            Messages::BlockHeader ih = ident->head();
            ih.set_previous_hash(head->GetPreviousHash());
            ih.set_hash(head->GetHash());
            ih.set_merkle_root(head->GetMerkleRoot());
            ih.set_height(head->GetHeight());
            Message resp(Message::Type::kIdentMessage, ident);
            Send(stream, &resp);
        } else{
            LOG(ERROR) << "unknown message type: " << static_cast<int>(msg->GetType());
        }
        return true;
    }
}