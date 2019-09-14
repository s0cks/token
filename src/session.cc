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

    void PeerSession::SendIdentity(){
        Node::Messages::PeerIdentity ident;
        ident.set_version("1.0.0");
        if(BlockChain::GetInstance()->HasHead()){
            ident.add_heads(BlockChain::GetInstance()->GetHead()->GetHash());
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

        Message msg(Message::Type::kPeerIdentityMessage, &ident);
        Send(&msg);
    }

    bool PeerSession::AcceptsIdentity(Node::Messages::PeerIdentity* ident){
        return false;
    }

    bool PeerSession::Handle(uv_stream_t* stream, Token::Message *msg){
        if(msg->GetType() == Message::Type::kBlockMessage){
            Token::Block* block = Token::Block::Load(msg->GetAsBlock());
            LOG(INFO) << "received block: " << block->GetHash();
            if(!BlockChain::GetInstance()->AppendBlock(block)){
                LOG(ERROR) << "couldn't append block: " << block->GetHash();
                return false;
            }
            Message m(Message::Type::kBlockMessage, block->GetAsMessage());
            BlockChainServer::Broadcast(&m);
            return true;
        } else if(msg->GetType() == Message::Type::kGetHeadMessage){
            Token::Block* block = BlockChain::GetInstance()->GetHead();
            LOG(INFO) << "sending head: " << block->GetHash();
            Message response(Message::Type::kBlockMessage, block->GetAsMessage());
            Send(&response);
            return true;
        } else if(msg->GetType() == Message::Type::kGetBlockMessage){
            Node::Messages::GetBlockRequest* req = msg->GetAsGetBlock();

            Block* blk;
            if(!req->hash().empty()){
                if(!(blk = BlockChain::GetInstance()->GetBlock(req->hash()))){
                    LOG(ERROR) << "peer requested block: " << req->hash() << ", which was not found";
                    return false;
                }
            } else if(req->height() > 0){
                if(!(blk = BlockChain::GetInstance()->GetBlock(req->height()))){
                    LOG(ERROR) << "peer requested block: @" << req->height() << ", which was not found";
                    return false;
                }
            } else{
                LOG(ERROR) << "invalid request";
                return false;
            }

            LOG(INFO) << "found peer's block, returning: " << blk->GetHash();
            Message m(Message::Type::kBlockMessage, blk->GetAsMessage());
            Send(&m);
            return true;
        } else if(msg->IsPeerIdentityMessage()){
            Node::Messages::PeerIdentity* ident = msg->GetAsPeerIdentity();
            if(IsConnected()){
                LOG(ERROR) << "already connected";
                return false;
            }

            if(ident->version() != "1.0.0"){
                LOG(ERROR) << "invalid version"; // TODO: Fixme
                return false;
            }

            std::string head = ident->heads(0);
            if(!head.empty() && head != BlockChain::GetInstance()->GetHead()->GetHash()) {
                LOG(ERROR) << "remote/<HEAD> != local/<HEAD>";
                SendIdentity();
                return true;
            } else if(head.empty()){
                LOG(INFO) << "sending <HEAD>";
                SendIdentity();
                return true;
            }

            LOG(INFO) << "connected";
            SetState(State::kConnected);
            return true;
        }

        LOG(ERROR) << "unknown message type: " << static_cast<int>(msg->GetType());
        return false;
    }

    void PeerSession::OnMessageSent(uv_write_t* req, int status) {
        if (status != 0) {
            std::cerr << "Error writing message" << std::endl;
            return;
        }
        if(req) free(req);
    }
}