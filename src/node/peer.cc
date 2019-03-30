#include <node/node.h>
#include "node/peer.h"

namespace Token{
    namespace Node{
        static void
        AllocBuffer(uv_handle_t* handle, size_t suggested_size, uv_buf_t* buff){
            Peer* p = (Peer*) handle->data;
            ByteBuffer* read = p->GetReadBuffer();
            *buff = uv_buf_init((char*) read->GetBytes(), suggested_size);
        }

        void Peer::Connect(){
            std::cout << "Connecting" << std::endl;
            uv_tcp_init(loop_, &stream_);
            uv_ip4_addr(address_.c_str(), port_, &addr_);
            conn_.data = (void*)this;
            uv_tcp_connect(&conn_, &stream_, (const struct sockaddr*)&addr_, [](uv_connect_t* conn, int status){
                Peer* p = (Peer*) conn->data;
                p->OnConnect(conn, status);
            });
        }

        void Peer::OnMessageRead(uv_stream_t *stream, ssize_t nread, const uv_buf_t *buff){
            Peer* p = (Peer*) stream->data;
            if(nread == UV_EOF){
                std::cerr << "Disconnected from peer: " << (*p) << std::endl;
                uv_read_stop(stream);
            } else if(nread > 0){
                p->GetReadBuffer()->PutBytes((uint8_t*) buff->base, buff->len);
                Message* m = p->GetNextMessage();
                if(m->IsBlock()){

                } else if(m->IsAppendBlock()){
                    Block* b = m->AsAppendBlock()->GetBlock();
                    std::cout << "Appending:" << std::endl;
                    std::cout << (*b) << std::endl;
                    if(Node::Server::GetInstance()->GetBlockChain()->Append(b)){
                        Node::Server::GetInstance()->Broadcast(stream, m);
                    }
                } else if(m->IsSyncRequest()){
                    SyncRequestMessage m;
                    SendTo(stream, &m);
                } else if(m->IsSyncResponse()){
                    Block* block = m->AsSyncResponse()->GetBlock();
                    std::cout << "Synchronizing to:" << std::endl << (*block) << std::endl;
                    Server::GetInstance()->GetBlockChain()->SetHead(block);
                }
            }
        }

        void Peer::SendTo(uv_stream_t *stream, Token::Message *msg){
            ByteBuffer bb;
            msg->Encode(&bb);

            uv_buf_t buff = uv_buf_init((char*) bb.GetBytes(), bb.WrittenBytes());
            uv_write_t* req = (uv_write_t*)malloc(sizeof(uv_write_t*));
            req->data = (void*)this;
            uv_write(req, stream, &buff, 1, [](uv_write_t* req, int status){
                Peer* p = (Peer*)req->data;
                p->OnMessageSend(req, status);
            });
        }

        void Peer::Send(Token::Message* msg){
            std::cout << "Sending: " << msg->GetName() << std::endl;

            ByteBuffer bb;
            msg->Encode(&bb);

            uv_buf_t buff = uv_buf_init((char*)bb.GetBytes(), bb.WrittenBytes());
            uv_write_t* req = (uv_write_t*)malloc(sizeof(uv_write_t));
            req->data = (void*)this;
            uv_write(req, handle_, &buff, 1, [](uv_write_t* req, int status){
                Peer* p = (Peer*) req->data;
                p->OnMessageSend(req, status);
            });
        }

        void Peer::OnMessageSend(uv_write_t *req, int status){
            if(status == 0){
                if(req) free(req);
            } else{
                std::cerr << "Error writing message" << std::endl;
            }
        }

        void Peer::OnConnect(uv_connect_t *conn, int status){
            if(status == 0){
                std::cout << "Connected" << std::endl;
                state_ = State::kConnected;
                handle_ = conn->handle;
                handle_->data = conn->data;
                uv_read_start(handle_, AllocBuffer, [](uv_stream_t* stream, ssize_t nread, const uv_buf_t* buff){
                    Peer* p = (Peer*) stream->data;
                    p->OnMessageRead(stream, nread, buff);
                });
            } else{
                std::cerr << "Connection unsuccessful" << std::endl;
                std::cerr << uv_strerror(status) << std::endl;
            }
        }
    }
}