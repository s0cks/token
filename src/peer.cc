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

    PeerClient::PeerClient(const std::string &addr, int port):
        loop_(uv_loop_new()),
        address_(addr),
        port_(port),
        stream_(),
        connection_(),
        state_(State::kConnecting),
        handle_(nullptr){}

    PeerClient::~PeerClient(){}

    bool PeerClient::AcceptsIdentity(Token::Node::Messages::PeerIdentity* pident){
        return true;
    }

    void PeerClient::OnConnect(uv_connect_t *conn, int status) {
        if (status == 0) {
            BlockChainServer::Register(this);
            state_ = State::kConnecting;
            handle_ = conn->handle;
            handle_->data = conn->data;
            uv_read_start(handle_, AllocBuffer, [](uv_stream_t *stream, ssize_t nread, const uv_buf_t *buff) {
                PeerClient *peer = (PeerClient*)stream->data;
                peer->OnMessageRead(stream, nread, buff);
            });
        }
    }

    bool PeerClient::Handle(uv_stream_t* stream, Token::Message* msg) {
        LOG(INFO) << "handling: " << msg->ToString() << "...";
        if(msg->GetType() == Message::Type::kPeerIdentMessage){
            if(GetState() != State::kConnecting && GetState() != State::kAuthenticating){
                LOG(ERROR) << "peer state is not connecting and we received a peer identity pkt...";
                LOG(ERROR) << "discarding..";
                return false;
            }
            Node::Messages::PeerIdentity* pident = msg->GetAsPeerIdentMessage();
            if(!AcceptsIdentity(pident)){
                LOG(ERROR) << "connection from weird peer: " << msg->ToString();
                return false;
            }

            LOG(INFO) << "connection attempt started, sending identity";
            Node::Messages::PeerIdentity mident;
            mident.set_version("1.0.0");
            mident.set_block_count(BlockChain::GetInstance()->GetHead()->GetHeight());
            Message m(Message::Type::kPeerIdentMessage, &mident);
            Send(&m);
            return true;
        } else if(msg->GetType() == Message::Type::kPeerIdentAckMessage){
            LOG(INFO) << "peer identity acknowledged";
            if(GetState() != State::kConnecting && GetState() != State::kAuthenticating){
                LOG(ERROR) << "peer not in connecting state";
                return false;
            }

            Node::Messages::PeerIdentAck* ack = msg->GetAsPeerIdentAckMessage();
            if(ack && ack->blocks_size() > 0){
                SetState(State::kAuthenticating);
                LOG(WARNING) << "peer responded with a boostrap";
                LOG(WARNING) << "adding peers";

                for(auto& it : ack->peers()){
                    std::string peer = it.address();
                    LOG(WARNING) << "adding peer: " << peer;
                    if(peer.find(":") != std::string::npos){
                        std::string addr = peer.substr(0, peer.find(":"));
                        std::string p = peer.substr(peer.find(":") + 1);
                        BlockChainServer::AddPeer(addr, atoi(p.c_str()));
                    }
                }

                LOG(WARNING) << "downloading <HEAD>s.....";
                for(auto& it : ack->blocks()){
                    Node::Messages::GetBlockRequest req;
                    req.set_hash(it);

                    Message request(Message::Type::kGetBlockMessage, &req);
                    Send(&request);
                }

                Node::Messages::PeerIdentity ident;
                ident.set_version("1.0.0");
                ident.set_block_count(BlockChain::GetInstance()->GetHeight());
                Message done(Message::Type::kPeerIdentMessage, &ident);
                Send(&done);
                return true;
            }
            LOG(INFO) << "connected";
            SetState(State::kConnected);
            return true;
        } else if(msg->GetType() == Message::Type::kBlockMessage){
            Token::Block* block = Token::Block::Load(msg->GetAsBlockMessage());
            if(GetState() == State::kAuthenticating){
                LOG(WARNING) << "downloaded block: " << block->GetHash();
                if(!BlockChain::GetInstance()->SetHead(block)){
                    LOG(ERROR) << "couldn't set head to: " << block->GetHash();
                    return false;
                }
                return true;
            } else if(GetState() == State::kConnected){
                LOG(INFO) << "received block: " << block->GetHash();
                if(!BlockChain::GetInstance()->Append(block)){
                    LOG(ERROR) << "couldn't append block: " << block->GetHash();
                    return false;
                }
                Message m(Message::Type::kBlockMessage, block->GetAsMessage());
                BlockChainServer::GetInstance()->Broadcast(nullptr, &m);
                return true;
            }
            LOG(ERROR) << "invalid state to receive block";
            return false;
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