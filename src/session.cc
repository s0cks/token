#include <glog/logging.h>
#include <sys/time.h>

#include "session.h"
#include "blockchain.h"
#include "server.h"

namespace Token {
    static uint32_t
    GetCurrentTime(){
        struct timeval time;
        gettimeofday(&time, NULL);
        uint32_t curr_time = ((uint32_t)time.tv_sec * 1000 + time.tv_usec / 1000);
        return curr_time;
    }

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

    bool PeerSession::Handle(uv_stream_t* stream, Token::Message *msg){
        LOG(INFO) << "handling " << msg->ToString() << " message...";
        if(msg->IsVersionMessage()){
            LOG(INFO) << "peer attempting to connect";
            if(!IsConnecting()){
                LOG(ERROR) << "peer already connected!";
                return false;
            }
            Node::Messages::Version* version = msg->GetAsVersion();
            if(version->version() != "1.0.0"){
                LOG(ERROR) << "peer disconnecting, unsupported version";
                return false;
            }
            if(version->height() == -1 || !BlockChain::GetHead()->Equals(version->head())){
                LOG(WARNING) << "peer connecting with mismatch <HEAD>, sending blocks";

                Node::Messages::BlockList blocks;
                if(!BlockChain::GetBlockList(&blocks)){
                    LOG(ERROR) << "couldn't get block list";
                    return false;
                }

                Message m(Message::Type::kBlockListMessage, &blocks);
                Send(&m);
                return true;
            }

            Node::Messages::Verack verack;
            verack.set_time(GetCurrentTime());
            verack.set_version("1.0.0");
            Message m(Message::Type::kVerackMessage, &verack);
            Send(&m);
            return true;
        }

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