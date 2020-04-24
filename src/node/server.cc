#include <glog/logging.h>
#include <string.h>
#include "block_miner.h"
#include "node/server.h"
#include "layout.h"
#include "message_data.h"

namespace Token{
    static uv_loop_t* loop = nullptr;
    static uv_tcp_t server;
    static uv_async_t broadcast_handle_;

    void BlockChainServer::AllocBuffer(uv_handle_t* handle, size_t suggested_size, uv_buf_t* buff){
        BlockChainServer* instance = GetInstance();
        buff->base = (char*)malloc(suggested_size);
        buff->len = suggested_size;
    }

    BlockChainServer::BlockChainServer():
        sessions_(),
        peers_(),
        thread_(){}

    BlockChainServer* BlockChainServer::GetInstance(){
        static BlockChainServer instance;
        return &instance;
    }

    void BlockChainServer::OnNewConnection(uv_stream_t* stream, int status){
        BlockChainServer* instance = GetInstance();
        LOG(INFO) << "client connecting....";
        if(status != 0){
            LOG(ERROR) << "connection error: " << std::string(uv_strerror(status));
            return;
        }
        PeerSession* session = new PeerSession();
        instance->sessions_.insert(std::make_pair((uv_stream_t*)session->GetHandle(), session));
        uv_tcp_init(loop, session->GetHandle());
        uv_timer_init(loop, &session->ping_timer_);
        uv_timer_init(loop, &session->timeout_timer_);
        if((status = uv_accept(stream, (uv_stream_t*)session->GetHandle())) != 0){
            LOG(ERROR) << "error accepting connection: " << uv_strerror(status);
            return;
        }
        session->SetState(SessionState::kConnecting);
        LOG(INFO) << "client accepted!";
        uv_read_start(
                (uv_stream_t*)session->GetStream(),
                &AllocBuffer,
                &OnMessageReceived
        );
    }

    void BlockChainServer::OnPing(uv_timer_t* handle){
        PeerSession* session = (PeerSession*)handle->data;
        session->SendPing();
        uv_timer_start(&session->timeout_timer_, &OnTimeout, 30 * 1000, 0);
    }

    void BlockChainServer::OnTimeout(uv_timer_t* handle){
        PeerSession* session = (PeerSession*)handle->data;
        LOG(WARNING) << "session timed out!";
        //TODO: implement
    }

    void BlockChainServer::HandleVersion(uv_work_t* req){
        ProcessMessageData* data = (ProcessMessageData*)req->data;
        Session* session = data->session;
        if(!session->IsConnecting()){
            LOG(INFO) << "session not connecting";
            return;
        }
        session->SetState(SessionState::kHandshaking);
        session->SendVersion();
    }

    void BlockChainServer::HandleVerack(uv_work_t* req){
        ProcessMessageData* data = (ProcessMessageData*)req->data;
        PeerSession* session = (PeerSession*)data->session;
        VerackMessage* verack = data->request->AsVerackMessage();
        session->SendVerack();
        if(session->IsSynchronizing()){
            return;
        } else if(session->IsHandshaking()){
            uv_timer_start(&session->ping_timer_, &OnPing, 0, 30 * 1000);
            session->SetState(SessionState::kConnected);
            if(verack->GetMaxBlock() < BlockChain::GetHeight()) session->SetState(SessionState::kSynchronizing);
            return;
        }
        LOG(ERROR) << "session is not handshaking or synchronizing";
        return;
    }

    void BlockChainServer::HandleGetData(uv_work_t* req){
        ProcessMessageData* data = (ProcessMessageData*)req->data;
        Session* session = data->session;
        GetDataMessage* request = data->request->AsGetDataMessage();

        uint256_t hash = HashFromHexString(request->GetHash());
        Block block;
        if(!BlockChain::GetBlockData(hash, &block)){
            LOG(ERROR) << "couldn't find block: " << hash;
            return;
        }
        session->SendBlock(block);
    }

    void BlockChainServer::HandleGetBlocks(uv_work_t* req){
        ProcessMessageData* data = (ProcessMessageData*)req->data;
        Session* session = data->session;
        GetBlocksMessage* request = data->request->AsGetBlocksMessage();

        Proto::BlockChainServer::Inventory inventory;

        uint256_t firstHash = request->GetFirst();
        uint256_t lastHash = request->GetLast();

        BlockHeader firstHeader = BlockChain::GetBlock(firstHash);
        BlockHeader lastHeader;
        if(lastHash.IsNull()){
            lastHeader = BlockChain::GetHead();
        } else{
            lastHeader = BlockChain::GetBlock(lastHash);
        }

        BlockHeader current = lastHeader;
        while(!current.GetPreviousHash().IsNull()){
            uint256_t hash = current.GetHash();
            uint256_t next = current.GetPreviousHash();

            LOG(INFO) << "inventory: " << hash;

            inventory.add_items(HexString(hash));
            current = BlockChain::GetBlock(next);
        }
        session->SendInventory(inventory);
    }

    void BlockChainServer::HandleBlock(uv_work_t* req){
        BlockChainServer* instance = GetInstance();
        ProcessMessageData* data = (ProcessMessageData*)req->data;
        Block* block = data->request->AsBlockMessage()->GetBlock();
        if(!BlockChain::AppendBlock(block)){
            LOG(ERROR) << "couldn't append block: " << block->GetHash();
            return;
        }
    }

    void BlockChainServer::HandleTransaction(uv_work_t* req){
        BlockChainServer* instance = GetInstance();
        ProcessMessageData* data = (ProcessMessageData*)req->data;
        Transaction* tx = data->request->AsTransactionMessage()->GetTransaction();
        if(!TransactionPool::PutTransaction(tx)){
            LOG(ERROR) << "couldn't add transaction to pool: " << tx->GetHash();
            return;
        }
    }

    void BlockChainServer::HandlePong(uv_work_t* req){
        BlockChainServer* instance = GetInstance();
        ProcessMessageData* data = (ProcessMessageData*)req->data;
        PeerSession* session = (PeerSession*)data->session;
        PongMessage* msg = data->request->AsPongMessage();
        LOG(INFO) << "pong: " << msg->GetNonce();
        uv_timer_stop(&session->timeout_timer_);
    }

    void BlockChainServer::AfterHandleMessage(uv_work_t* req, int status){
        ProcessMessageData* data = (ProcessMessageData*)req->data;
        if(data->request) free(data->request);
        free(req->data);
        free(req);
    }

    void BlockChainServer::HandleBroadcastBlock(uv_work_t* req){
        BroadcastBlockData* data = (BroadcastBlockData*)req->data;
        Block* block = data->block;

        std::list<Session*> peers;
        BlockChainServer::GetConnectedPeers(peers);

        LOG(INFO) << "broadcasting: " <<  block->GetHash() << " to " << peers.size() << " peers....";
        for(auto& it : peers) it->SendBlock((*block));
    }

    void BlockChainServer::AfterBroadcastBlock(uv_work_t* req, int status){
        BroadcastBlockData* data = (BroadcastBlockData*)req->data;
        free(req->data);
        free(req);
    }

    void BlockChainServer::OnMessageReceived(uv_stream_t *stream, ssize_t nread, const uv_buf_t* buff){
        BlockChainServer* instance = GetInstance();
        auto pos = instance->sessions_.find(stream);
        if(pos == instance->sessions_.end()){
            LOG(ERROR) << "couldn't find session";
            return;
        }

        Session* session = pos->second;
        if(nread == UV_EOF){
            LOG(ERROR) << "client disconnected!";
            return;
        } else if(nread < 0){
            LOG(ERROR) << "[" << nread << "] client read error: " << std::string(uv_strerror(nread));
            return;
        } else if(nread == 0){
            LOG(WARNING) << "zero message size received";
            return;
        }

        if(nread >= 4096){
            LOG(ERROR) << "too large of a buffer";
            return;
        }

        ByteBuffer bb((uint8_t*)buff->base, nread);
        Message* msg;
        if(!(msg = Message::Decode(&bb))){
            LOG(INFO) << "couldn't decode message!";
            return;
        }

        uv_work_t* work = (uv_work_t*)malloc(sizeof(uv_work_t));
        work->data = new ProcessMessageData(session, msg);
        if(msg->IsVersionMessage()){
            uv_queue_work(stream->loop, work, HandleVersion, AfterHandleMessage);
        } else if(msg->IsGetDataMessage()){
            uv_queue_work(stream->loop, work, HandleGetData, AfterHandleMessage);
        } else if(msg->IsGetBlocksMessage()){
            uv_queue_work(stream->loop, work, HandleGetBlocks, AfterHandleMessage);
        } else if(msg->IsBlockMessage()){
            uv_queue_work(stream->loop, work, HandleBlock, AfterHandleMessage);
        } else if(msg->IsTransactionMessage()){
            uv_queue_work(stream->loop, work, HandleTransaction, AfterHandleMessage);
        } else if(msg->IsVerackMessage()){
            uv_queue_work(stream->loop, work, HandleVerack, AfterHandleMessage);
        } else if(msg->IsPongMessage()){
            uv_queue_work(stream->loop, work, HandlePong, AfterHandleMessage);
        }
    }

    void BlockChainServer::ConnectToPeer(const std::string &address, uint16_t port){
        ClientSession* session = new ClientSession(address, port);
        if(session->Connect() != 0){
            LOG(ERROR) << "couldn't connect to peer";
        }
        GetInstance()->peers_.push_front(session);
    }

    void BlockChainServer::BroadcastInventory(const uint256_t& inv){
        broadcast_handle_.data = nullptr; //inv
        uv_async_send(&broadcast_handle_);
    }

    void BlockChainServer::HandleAsyncBroadcastHead(uv_async_t* handle){
        BlockChainServer* instance = GetInstance();
        Block* block;
        if(!(block = (Block*)handle->data)){
            LOG(ERROR) << "invalid data?";
            return;
        }
        uv_work_t* work = (uv_work_t*)malloc(sizeof(uv_work_t));
        work->data = new BroadcastBlockData(block);
        uv_queue_work(handle->loop, work, HandleBroadcastBlock, AfterBroadcastBlock);
    }

    void* BlockChainServer::ServerThread(void* data){
        loop = uv_loop_new();
        uv_async_init(loop, &broadcast_handle_, HandleAsyncBroadcastHead);

        sockaddr_in addr;
        uv_ip4_addr("0.0.0.0", FLAGS_port, &addr);
        uv_tcp_init(loop, &server);
        uv_tcp_keepalive(&server, 1, 60);
        uv_tcp_bind(&server, (const struct sockaddr*)&addr, 0);
        int err;
        if((err = uv_listen((uv_stream_t*)&server, 100, &OnNewConnection)) != 0){
            LOG(ERROR) << "couldn't start server: " << uv_strerror(err);
            pthread_exit(nullptr);
        }
        LOG(INFO) << "server listening @ localhost:" << FLAGS_port;
        uv_run(loop, UV_RUN_DEFAULT);
        pthread_exit(nullptr);
    }

    bool BlockChainServer::StartServer(){
        BlockChainServer* instance = GetInstance();
        return pthread_create(&instance->thread_, NULL, &ServerThread, (void*)instance) == 0;
    }

    void BlockChainServer::WaitForShutdown(){
        BlockChainServer* instance = GetInstance();
        void* result = nullptr;
        pthread_join(instance->thread_, &result);
    }
}