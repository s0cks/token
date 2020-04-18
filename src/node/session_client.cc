#include "node/session.h"
#include "layout.h"
#include "message_data.h"
#include "block_miner.h"
#include "node/node.h"

namespace Token {
    static const size_t kResolveWaitTimeMilliseconds = 1 * 1000;

    void ClientSession::ResolveInventory(uv_work_t* req){
        ResolveInventoryData* data = (ResolveInventoryData*)req->data;
        Session* session = data->session;
        std::string hash = data->hashes.back();
        if(BlockChain::ContainsBlock(HashFromHexString(hash))){
            LOG(INFO) << hash << " found!";
            return;
        }
        LOG(INFO) << hash << " not found, resolving....";
        session->SendGetData(hash);
        uv_sleep(kResolveWaitTimeMilliseconds); // Add 1 second wait to resolve blocks
    }

    void ClientSession::AfterResolveInventory(uv_work_t* req, int status){
        ResolveInventoryData* data = (ResolveInventoryData*)req->data;
        Session* session = data->session;
        std::string hash = data->hashes.front();
        if(!BlockChain::ContainsBlock(HashFromHexString(hash))){
            LOG(INFO) << hash << " wasn't downloading rescheduling";
            uv_queue_work(req->loop, req, ResolveInventory, AfterResolveInventory);
            return;
        }
        data->hashes.back();
        data->hashes.pop_back();
        if(!data->hashes.empty()){
            LOG(INFO) << "downloading remaining hashes....";
            uv_queue_work(req->loop, req, ResolveInventory, AfterResolveInventory);
            return;
        }
        session->SendVerack();
    }

    void ClientSession::HandleInventoryMessage(uv_work_t* req){
        ProcessMessageData* data = (ProcessMessageData*)req->data;
        Session* session = data->session;
        InventoryMessage* inventory = data->request->AsInventoryMessage();
        session->SetState(SessionState::kSynchronizing);

        std::vector<std::string> hashes;
        if(!inventory->GetHashes(hashes)){
            LOG(ERROR) << "couldn't get hashes from inventory";
            return;
        }

        uv_work_t* work = (uv_work_t*)malloc(sizeof(uv_work_t));
        work->data = new ResolveInventoryData(session, hashes);
        uv_queue_work(req->loop, work, ResolveInventory, AfterResolveInventory);
    }

    void ClientSession::HandleBlockMessage(uv_work_t* req){
        ProcessMessageData* data = (ProcessMessageData*)req->data;
        Session* session = data->session;
        Block* block = data->request->AsBlockMessage()->GetBlock();
        LOG(INFO) << "received block: " << block->GetHash();
        if(!BlockChain::AppendBlock(block)){
            LOG(WARNING) << "couldn't schedule block for mining: " << block->GetHash();
            return;
        }
    }

    void ClientSession::HandleVersionMessage(uv_work_t* req){
        ProcessMessageData* data = (ProcessMessageData*)req->data;
        Session* session = data->session;
        if(!session->IsConnecting()){
            LOG(ERROR) << "session is not handshaking";
            return;
        }
        session->SetState(SessionState::kHandshaking);
        session->SendVerack();
    }

    void ClientSession::HandleVerackMessage(uv_work_t* req){
        ProcessMessageData* data = (ProcessMessageData*)req->data;
        Session* session = data->session;
        VerackMessage* verack = data->request->AsVerackMessage();
        if(!(session->IsHandshaking() || session->IsSynchronizing())){
            LOG(ERROR) << "session isn't handshaking";
            return;
        }

        BlockHeader head = BlockChain::GetHead();
        if(head.GetHeight() < verack->GetMaxBlock()){
            LOG(INFO) << "synchronizing " << verack->GetMaxBlock() << " blocks....";
            session->SetState(SessionState::kSynchronizing);
            session->SendGetBlocks(HexString(head.GetHash()), HexString(uint256_t()));
            return;
        }

        LOG(INFO) << "connected!";
        session->SetState(SessionState::kConnected);
    }

    void ClientSession::AfterProcessMessage(uv_work_t* req, int status){
        ProcessMessageData* data = (ProcessMessageData*)req->data;
        if(data->request) free(data->request);
        free(req->data);
        free(req);
    }

    void ClientSession::OnMessageReceived(uv_stream_t *stream, ssize_t nread, const uv_buf_t *buff) {
        ClientSession *session = (ClientSession*)stream->data;
        if (nread == UV_EOF) {
            LOG(ERROR) << "disconnected from peer: " << uv_strerror(nread);
            return;
        } else if (nread > 0) {
            ByteBuffer* rbuffer = session->GetReadBuffer();
            Message::Type type = static_cast<Message::Type>(rbuffer->GetInt());
            uint64_t size = rbuffer->GetLong();

            uint8_t mbytes[size];
            rbuffer->GetBytes(mbytes, size);
            rbuffer->clear();

            Message* msg;
            if(!(msg = Message::Decode(type, mbytes, size))){
                LOG(ERROR) << "couldn't decode message from byte array";
                return;
            }

            uv_work_t* work = (uv_work_t*)malloc(sizeof(uv_work_t));
            work->data = new ProcessMessageData(session, msg);
            if(msg->IsInventoryMessage()){
                uv_queue_work(stream->loop, work, HandleInventoryMessage, AfterProcessMessage);
            } else if(msg->IsBlockMessage()){
                uv_queue_work(stream->loop, work, HandleBlockMessage, AfterProcessMessage);
            } else if(msg->IsVersionMessage()){
                uv_queue_work(stream->loop, work, HandleVersionMessage, AfterProcessMessage);
            } else if(msg->IsVerackMessage()){
                uv_queue_work(stream->loop, work, HandleVerackMessage, AfterProcessMessage);
            }
        }
    }

    void ClientSession::OnConnect(uv_connect_t *conn, int status) {
        ClientSession *session = (ClientSession *) conn->data;
        if (status != 0) {
            LOG(ERROR) << "error connecting: " << uv_strerror(status);
            return;
        }
        session->SetState(SessionState::kConnecting);
        session->handle_ = conn->handle;
        session->handle_->data = session;
        uv_read_start(session->handle_, AllocBuffer, &OnMessageReceived);
        session->SendVersion();
    }

    void* ClientSession::ClientSessionThread(void* data){
        ClientSession* session = (ClientSession*)data;
        std::cerr << "creating client session....";
        uv_loop_t* loop = uv_loop_new();
        uv_tcp_init(loop, &session->stream_);
        struct sockaddr_in addr;
        uv_ip4_addr(session->GetAddress(), session->GetPort(), &addr);
        session->conn_.data = (void*)session;
        int err = uv_tcp_connect(&session->conn_, &session->stream_, (const struct sockaddr*)&addr, &OnConnect);
        if(err != 0){
            LOG(ERROR) << "cannot connect to peer: " << uv_strerror(err);
        }
        uv_run(loop, UV_RUN_DEFAULT);
        return nullptr;
    }

    int ClientSession::Connect() {
        LOG(INFO) << "connecting to: " << GetAddress() << ":" << GetPort();
        return pthread_create(&thread_, NULL, &ClientSessionThread, (void *) this);
    }
}