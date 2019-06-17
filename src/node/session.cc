#include "node/session.h"
#include <iostream>
#include "node/node.h"

namespace Token{
    static void
    AllocBuffer(uv_handle_t* handle, size_t suggested_size, uv_buf_t* buff){
        Session* session = (Session*) handle->data;
        session->InitializeBuffer(buff, suggested_size);
    }

#define DEFINE_SESSION_TYPECHECK(Name) \
    Name##Session* Name##Session::As##Name##Session(){ return this; }
    FOR_EACH_SESSION_TYPE(DEFINE_SESSION_TYPECHECK)
#undef DEFINE_SESSION_TYPECHECK

    void Session::OnMessageSent(uv_write_t* request, int status){
        if(status != 0){
            std::cerr << "Error writing message" << std::endl;
            return;
        }
        if(request) free(request);
    }

    bool
    Session::Send(Message* msg) {
        ByteBuffer bb;
        msg->Encode(&bb);
        uv_buf_t buff = uv_buf_init((char *) bb.GetBytes(), bb.WrittenBytes());
        uv_write_t *req = (uv_write_t *) malloc(sizeof(uv_write_t));
        req->data = this;
        uv_write(req, GetStream(), &buff, 1, [](uv_write_t *req, int status) {
            Session *session = (Session *) req->data;
            session->OnMessageSent(req, status);
        });
        return true;
    }

    bool ClientSession::Handle(Token::Message* msg){
        if(msg->IsBlock()){
            Block* block = msg->AsBlock()->GetBlock();
            std::cout << "Received block:" << std::endl;
            std::cout << (*block) << std::endl;
            std::cout << "broadcasting..." << std::endl;
            BlockChainServer::GetInstance()->Broadcast(GetStream(), msg);
            return true;
        }
        return false;
    }

    void PeerSession::Connect(){
        uv_tcp_init(loop_, &stream_);
        uv_ip4_addr(address_.c_str(), port_, &addr_);
        connection_.data = this;
        uv_tcp_connect(&connection_, &stream_, (const struct sockaddr*)&addr_, [](uv_connect_t* conn, int status){
            PeerSession* session = (PeerSession*)conn->data;
            session->OnConnect(conn, status);
        });
    }

    void PeerSession::OnConnect(uv_connect_t *conn, int status){
        if(status == 0){
            std::cout << "Connected" << std::endl;
            state_ = State::kConnected;
            handle_ = conn->handle;
            handle_->data = this;
            uv_read_start(handle_, AllocBuffer, [](uv_stream_t* stream, ssize_t nread, const uv_buf_t* buff){
                PeerSession* session = (PeerSession*)stream->data;
                session->OnMessageRead(stream, nread, buff);
            });
        }
    }

    bool PeerSession::Handle(Token::Message* msg){
        if(msg->IsBlock()){
            Block* block = msg->AsBlock()->GetBlock();
            std::cout << (*this) << " sent block:" << std::endl;
            std::cout << (*block) << std::endl;
            BlockChainServer::GetInstance()->Broadcast(GetStream(), msg);
            return true;
        }
        return false;
    }

    void PeerSession::OnMessageRead(uv_stream_t *stream, ssize_t nread, const uv_buf_t *buff){
        PeerSession* session = (PeerSession*)stream->data;
        std::cout << "read " << nread << " bytes" << std::endl;
        if(nread == UV_EOF){
            std::cerr << "disconnected from peer" << std::endl;
            uv_read_stop(stream);
        } else if(nread > 0){
            session->Append(buff);
            Message* msg = session->GetNextMessage();
            if(!session->Handle(msg)){
                std::cerr << "couldn't handle message" << std::endl;
            }
        }
    }
}