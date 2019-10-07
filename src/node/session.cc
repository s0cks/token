#include <glog/logging.h>

#include "node/session.h"
#include "blockchain.h"
#include "node/server.h"

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

    void PeerSession::SendBlock(const std::string &block){
        Block* blk;
        if(!(blk = BlockChain::GetBlock(block))){
            LOG(ERROR) << "couldn't find block: " << block;
            return;
        }

        Message msg(Message::Type::kBlockMessage, blk->GetAsMessage());
        Send(&msg);
    }

    void PeerSession::SendInventory(){
        std::vector<std::string> hashes;
        if(!BlockChain::GetBlockList(hashes)){
            LOG(ERROR) << "couldn't get block list";
            return;
        }
        std::reverse(hashes.begin(), hashes.end());
        Messages::HashList blocks;
        for(auto& it : hashes){
            blocks.add_hashes(it);
        }
        Message msg(Message::Type::kInventoryMessage, &blocks);
        Send(&msg);
    }

    bool PeerSession::Handle(uv_stream_t* stream, Token::Message *msg){
        if(IsConnecting()){
            if(msg->IsVersionMessage()){
                LOG(INFO) << "received version:";
                Messages::Node::Version* version = msg->GetAsVersion();
                LOG(INFO) << "- Timestamp: " << version->time();
                LOG(INFO) << "- Nonce: " << version->nonce();
                LOG(INFO) << "- Version: " << version->version();
                LOG(INFO) << "- Blocks: " << version->blocks();
                if(BlockChain::GetHeight() >= version->blocks()){
                    LOG(INFO) << "accepted!";
                    SendVersion(version->nonce());
                    return true;
                } else{
                    LOG(INFO) << "rejected!";
                    SetState(State::kDisconnected);
                    return false;
                }
            } else if(msg->IsGetDataMessage()){
                Messages::HashList* req = msg->GetAsGetData();

                Block* lhead = BlockChain::GetHead();
                Block* phead = BlockChain::GetBlock(req->hashes(0));
                if(!phead){
                    LOG(WARNING) << "peer returned unknown block, sending inventory...";
                    SetState(State::kSynchronizing);
                    SendInventory();
                    return true;
                }

                if((*lhead) != (*phead)){
                    LOG(WARNING) << "local/<HEAD> != remote/<HEAD>";
                    SetState(State::kSynchronizing);
                    SendInventory();
                    return true;
                }

                LOG(INFO) << "connected!";
                SetState(State::kConnected);
                SendBlock(lhead->GetHash());
                return true;
            } else{
                LOG(ERROR) << "invalid message type for connecting state: " << msg->ToString();
                return false;
            }
        } else if(IsSynchronizing()){
            if(msg->IsGetDataMessage()){
                std::string lhead = BlockChain::GetHead()->GetHash();
                LOG(INFO) << "getting: " << msg->ToString();
                Messages::HashList* hashes = msg->GetAsGetData();
                for(auto& it : hashes->hashes()){
                    SendBlock(it);
                    if(it == lhead){
                        LOG(INFO) << "sent <HEAD>, attempting to connecting again";
                        SetState(State::kConnecting);
                    }
                }
                return true;
            }
        }

        LOG(INFO) << "handling " << msg->ToString() << " message...";
        if(msg->GetType() == Message::Type::kBlockMessage){
            Token::Block* block = Token::Block::Decode(msg->GetAsBlock());
            LOG(INFO) << "received block: " << block->GetHash();
            if(!BlockChain::GetInstance()->AppendBlock(block)){
                LOG(ERROR) << "couldn't append block: " << block->GetHash();
                return false;
            }
            Message m(Message::Type::kBlockMessage, block->GetAsMessage());
            BlockChainServer::Broadcast(&m);
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

    void PeerSession::SendVersion(const std::string& nonce){
        Messages::Node::Version version;
        version.set_time(GetCurrentTime());
        version.set_version("1.0.0");
        version.set_nonce(nonce);
        version.set_blocks(BlockChain::GetHeight());
        Message msg(Message::Type::kVersionMessage, &version);
        Send(&msg);
    }
}