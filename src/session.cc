#include <glog/logging.h>
#include "session.h"
#include "blockchain.h"
#include "server.h"

namespace Token {
    bool PeerSession::Send(Token::Message *msg) {
        size_t size = msg->GetMessageSize() + (sizeof(uint32_t) * 2);

        LOG(WARNING) << "sending: " << msg->ToString();
        uint8_t bytes[size];
        msg->Encode(bytes, size);

        uv_buf_t buff = uv_buf_init((char*)bytes, size);
        uv_write_t *req = (uv_write_t *) malloc(sizeof(uv_write_t));
        req->data = (void *) this;
        uv_write(req, (uv_stream_t*)GetConnection(), &buff, 1, [](uv_write_t *req, int status){
            PeerSession* client = (PeerSession*)req->data;
            client->OnMessageSent(req, status);
        });
        return true;
    }

    bool PeerSession::PerformHandshake(){
        if(GetState() != State::kConnecting){
            LOG(WARNING) << "state != connecting";
            return false;
        }

        Node::Messages::PeerIdentity ident;
        ident.set_version("1.0.0");
        ident.set_block_count(BlockChain::GetInstance()->GetHeight());
        Message pck(Message::Type::kPeerIdentMessage, &ident);
        Send(&pck);
        return true;
    }

    bool PeerSession::AcceptsIdentity(Node::Messages::PeerIdentity* ident){
        LOG(INFO) << "peer count: " << ident->peer_count();
        return false;
    }

    bool PeerSession::Handle(uv_stream_t* stream, Token::Message *msg){
        if(msg->GetType() == Message::Type::kBlockMessage){
            Token::Block* block = Token::Block::Load(msg->GetAsBlockMessage());
            LOG(INFO) << "received block: " << block->GetHash();
            if(!BlockChain::GetInstance()->Append(block)){
                LOG(ERROR) << "couldn't append block: " << block->GetHash();
                return false;
            }
            Message m(Message::Type::kBlockMessage, block->GetAsMessage());
            BlockChainServer::Broadcast(&m);
        } else if(msg->GetType() == Message::Type::kGetHeadMessage){
            Token::Block* block = BlockChain::GetInstance()->GetHead();
            LOG(INFO) << "sending head: " << block->GetHash();
            Message response(Message::Type::kBlockMessage, block->GetAsMessage());
            Send(&response);
        } else if(msg->GetType() == Message::Type::kPeerIdentMessage){
            if(GetState() != State::kAuthenticating && GetState() != State::kConnecting){
                LOG(ERROR) << "peer already connected?";
                return false;
            }
            Node::Messages::PeerIdentity* pident = msg->GetAsPeerIdentMessage();

            Node::Messages::PeerIdentAck ack;
            if(!AcceptsIdentity(pident)){
                SetState(State::kAuthenticating);
                if(pident->peer_count() < BlockChainServer::GetPeerCount() + 1){
                    LOG(INFO) << "sending connecting peer connected peers";
                    std::vector<std::string> peers;
                    if(!BlockChainServer::GetPeers(peers)){
                        LOG(INFO) << "couldn't get peer list";
                        return false;
                    }
                    LOG(INFO) << "peers (" << peers.size() << ": ";
                    for(auto& it : peers){
                        LOG(INFO) << "  - " << it;
                        ack.add_peers()->set_address(it);
                    }
                }
                LOG(INFO) << "sending <HEAD>";
                ack.add_blocks(BlockChain::GetInstance()->GetHead()->GetHash());
            } else{
                LOG(INFO) << "peer connected";
                SetState(State::kConnected);
            }

            Message resp(Message::Type::kPeerIdentAckMessage, &ack);
            Send(&resp);
            return true;
        } else if(msg->GetType() == Message::Type::kGetBlockMessage){
            Node::Messages::GetBlockRequest* req = msg->GetAsGetBlockMessage();

            Block* blk;
            if(!(blk = BlockChain::GetInstance()->GetBlockFromHash(req->hash()))){
                LOG(ERROR) << "peer requested block: " << req->hash() << ", which was not found";
                return false;
            }

            LOG(INFO) << "found peer's block, returning: " << blk->GetHash();
            Message m(Message::Type::kBlockMessage, blk->GetAsMessage());
            Send(&m);
        } else{
            LOG(ERROR) << "unknown message type: " << static_cast<int>(msg->GetType());
        }
        return true;
    }

    void PeerSession::OnMessageSent(uv_write_t* req, int status) {
        if (status != 0) {
            std::cerr << "Error writing message" << std::endl;
            return;
        }
        if(req) free(req);
    }
}