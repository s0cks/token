#include "session.h"
#include "node/node.h"
#include "blockchain.h"

namespace Token {
#define DEFINE_SESSION_TYPECHECK(Name) \
    Name##Session* Name##Session::As##Name##Session(){ return this; }

    FOR_EACH_SESSION(DEFINE_SESSION_TYPECHECK)

#undef DEFINE_SESSION_TYPECHECK

    static void
    AllocBuffer(uv_handle_t *handle, size_t suggested_size, uv_buf_t *buff) {
        PeerSession *peer = (PeerSession *) handle->data;
        peer->InitializeBuffer(buff, suggested_size);
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
    Session::Send(Token::Message *msg) {
        std::cout << "Sending '" << msg->GetName() << "'" << std::endl;
        ByteBuffer bb;
        msg->Encode(&bb);
        uv_buf_t buff = uv_buf_init((char *) bb.GetBytes(), bb.WrittenBytes());
        uv_write_t *req = (uv_write_t *) malloc(sizeof(uv_write_t *));
        req->data = (void *) this;
        uv_write(req, GetStream(), &buff, 1, [](uv_write_t *req, int status) {
            Session *session = (Session *) req->data;
            session->OnMessageSent(req, status);
        });
        return true;
    }

    void PeerSession::Connect() {
        std::cout << "Connecting to peer: " << (*this) << std::endl;
        uv_tcp_init(loop_, &stream_);
        uv_ip4_addr(addressstr_.c_str(), port_, &address_);
        connection_.data = (void *) this;
        uv_tcp_connect(&connection_, &stream_, (const struct sockaddr *) &address_, [](uv_connect_t *conn, int status) {
            PeerSession *peer = (PeerSession *) conn->data;
            peer->OnConnect(conn, status);
        });
    }

    void PeerSession::OnConnect(uv_connect_t *conn, int status) {
        if (status == 0) {
            std::cout << "Connected" << std::endl;
            state_ = State::kConnected;
            handle_ = conn->handle;
            handle_->data = conn->data;
            uv_read_start(handle_, AllocBuffer, [](uv_stream_t *stream, ssize_t nread, const uv_buf_t *buff) {
                PeerSession *peer = (PeerSession *) stream->data;
                peer->OnMessageRead(stream, nread, buff);
            });
        }
    }

    void PeerSession::OnMessageRead(uv_stream_t *stream, ssize_t nread, const uv_buf_t *buff) {
        PeerSession *peer = (PeerSession *) stream->data;
        if (nread == UV_EOF) {
            std::cerr << "Disconnected from peer: " << (*peer) << std::endl;
            uv_read_stop(stream);
        } else if (nread > 0) {
            peer->Append(buff);
            Message *msg = peer->GetNextMessage();
            if (!peer->Handle(msg)) {
                std::cerr << "Peer: " << (*peer) << " couldn't handle message: " << msg->GetName() << std::endl;
            }
        }
    }

    bool PeerSession::Handle(Token::Message *msg) {
        return true;
    }

    PeerSession::PeerSession(uv_loop_t *loop, std::string addr, int port) :
            loop_(loop),
            addressstr_(addr),
            port_(port),
            stream_(),
            connection_(),
            handle_(nullptr),
            address_(),
            Session() {
        Connect();
    }

    bool ClientSession::Handle(Token::Message *msg) {
        if (msg->IsRequest()) {
            Request *request = msg->AsRequest();
            if (msg->IsGetHeadRequest()) {
                std::string uuid = request->GetId();
                Block *head = BlockChain::GetInstance()->GetHead();
                GetHeadResponse response(uuid, head);
                Send(&response);
                return true;
            }
        }
        return false;
    }
}