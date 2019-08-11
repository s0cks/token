#include "node/session.h"
#include "blockchain.h"

namespace Token {
#define DEFINE_SESSION_TYPECHECK(Name) \
    Name##Session* Name##Session::As##Name##Session(){ return this; }
    FOR_EACH_SESSION(DEFINE_SESSION_TYPECHECK)
#undef DEFINE_SESSION_TYPECHECK

    static void
    AllocBuffer(uv_handle_t *handle, size_t suggested_size, uv_buf_t *buff) {
        buff->base = (char*)malloc(suggested_size);
        buff->len = suggested_size;
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
        std::cout << "Sending message of size: " << size << std::endl;
        uint8_t bytes[size];
        msg->Encode(bytes, size);
        uv_buf_t buff = uv_buf_init((char*)bytes, size);
        uv_write_t *req = (uv_write_t *) malloc(sizeof(uv_write_t));
        req->data = (void *) this;
        uv_write(req, GetStream(), &buff, 1, [](uv_write_t *req, int status) {
            Session *session = (Session *) req->data;
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
        std::cout << "Connecting to peer " << GetAddress() << ":" << GetPort() << "..." << std::endl;
        loop_ = uv_loop_new();
        uv_tcp_init(loop_, &stream_);
        uv_ip4_addr(addressstr_.c_str(), port_, &address_);
        connection_.data = (void*)this;
        int err = uv_tcp_connect(&connection_, &stream_, (const struct sockaddr *) &address_, [](uv_connect_t *conn, int status) {
            PeerSession *peer = (PeerSession *) conn->data;
            peer->OnConnect(conn, status);
        });
        if(err != 0){
            std::cerr << "cannot connect to peer: " << uv_strerror(err) << std::endl;
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
            std::cerr << "Unable to join peer thread" << std::endl;
            return;
        }
        printf("Peer disconnected with: %s\n", result);
    }

    void PeerSession::OnConnect(uv_connect_t *conn, int status) {
        std::cout << "Connecting: " << status << std::endl;
        if (status == 0) {
            std::cout << "Connected" << std::endl;
            GetParent()->Register(this);
            state_ = State::kConnected;
            handle_ = conn->handle;
            handle_->data = conn->data;
            uv_read_start(handle_, AllocBuffer, [](uv_stream_t *stream, ssize_t nread, const uv_buf_t *buff) {
                PeerSession *peer = (PeerSession*) stream->data;
                peer->OnMessageRead(stream, nread, buff);
            });
        }
    }

    void PeerSession::OnMessageRead(uv_stream_t *stream, ssize_t nread, const uv_buf_t *buff) {
        PeerSession *peer = (PeerSession *) stream->data;
        if (nread == UV_EOF) {
            std::cerr << "Disconnected from peer: " << (*peer) << std::endl;
            uv_read_stop(stream);
            return;
        } else if (nread > 0) {
            std::cout << "NRead: " << nread << "/" << buff->len << std::endl;
            peer->Append(buff, nread);

            Message msg;
            if(!msg.Decode(peer->GetReadBuffer())){
                std::cerr << "Couldn't decode message" << std::endl;
                uv_read_stop(stream);
                return;
            }
            peer->ClearReadBuffer();
            if(!peer->Handle(stream, &msg)){
                std::cerr << "Couldn't handle message" << std::endl;
                uv_read_stop(stream);
                return;
            }
        }
    }

    bool PeerSession::Handle(uv_stream_t* stream, Token::Message* msg) {
        if(msg->GetType() == Message::Type::kBlockMessage){
            Token::Block* block = Token::Block::Load(msg->GetAsBlockMessage());
            std::cout << "Received Block:" << std::endl;
            std::cout << (*block) << std::endl;
        }
        return true;
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
            Session(parent){}

    bool ClientSession::Handle(uv_stream_t* stream, Token::Message *msg){
        if(msg->GetType() == Message::Type::kBlockMessage){
            Token::Block* block = Token::Block::Load(msg->GetAsBlockMessage());
            std::cout << "Received block: " << (*block) << std::endl;
            if(!BlockChain::GetInstance()->Append(block)){
                std::cerr << "Couldn't append block" << std::endl;
                return false;
            }
            Message m(Message::Type::kBlockMessage, block->GetAsMessage());
            BlockChain::GetServerInstance()->BroadcastToPeers(stream, &m);
        } else if(msg->GetType() == Message::Type::kGetHeadMessage){
            Token::Block* block = BlockChain::GetInstance()->GetHead();
            std::cout << "Sending <HEAD>: " << (*block) << std::endl;
            Token::Messages::BlockHeader* blk = new Token::Messages::BlockHeader();
            Message response(Message::Type::kBlockMessage, block->GetAsMessage());
            Send(stream, &response);
        } else{
            std::cout << "Unknown message type: " << static_cast<int>(msg->GetType()) << std::endl;
        }
        return true;
    }
}