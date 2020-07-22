#include <algorithm>
#include <random>
#include <condition_variable>

#include "scope.h"
#include "server.h"
#include "message.h"
#include "task.h"
#include "configuration.h"

namespace Token{
//TODO: remove:
#define SCHEDULE(Loop, Name, ...)({ \
    uv_work_t* work = (uv_work_t*)malloc(sizeof(uv_work_t));\
    work->data = new Name##Task(__VA_ARGS__); \
    uv_queue_work((Loop), work, Handle##Name##Task, After##Name##Task); \
})

    static pthread_t thread_ = 0;
    static uv_tcp_t handle_;
    static uv_async_t aterm_;

    static std::recursive_mutex mutex_;
    static std::condition_variable_any cond_;
    static Server::State state_ = Server::State::kStopped;
    static NodeInfo info_ = NodeInfo();
    static std::map<std::string, PeerSession*> peers_ = std::map<std::string, PeerSession*>();

#define LOCK_GUARD std::lock_guard<std::recursive_mutex> guard(mutex_)
#define LOCK std::unique_lock<std::recursive_mutex> lock(mutex_)
#define WAIT cond_.wait(lock)
#define SIGNAL_ONE cond_.notify_one()
#define SIGNAL_ALL cond_.notify_all()

    void Server::LoadNodeInformation(){
        LOCK_GUARD;
        libconfig::Setting& node = BlockChainConfiguration::GetProperty("Server", libconfig::Setting::TypeGroup);
        //TODO: load callback address or fetch dynamically
        NodeAddress address("127.0.0.1", FLAGS_port);
        if(!node.exists("id")){
            info_ = NodeInfo(address);
            node.add("id", libconfig::Setting::TypeString) = info_.GetNodeID();
            BlockChainConfiguration::SaveConfiguration();
            return;
        }

        std::string node_id;
        node.lookupValue("id", node_id);
        info_ = NodeInfo(node_id, NodeAddress("127.0.0.1", FLAGS_port));
    }

    void Server::LoadPeers(){
        LOCK_GUARD;
        libconfig::Setting& peers = BlockChainConfiguration::GetProperty("Peers", libconfig::Setting::TypeArray);
        auto iter = peers.begin();
        while(iter != peers.end()){
            NodeAddress address((*iter));
            if(!HasPeer(address)){
                if(!ConnectTo(address)) LOG(WARNING) << "couldn't connect to peer: " << address;
            }
            iter++;
        }
    }

    void Server::SavePeers(){
        LOCK_GUARD;
        libconfig::Setting& root = BlockChainConfiguration::GetRoot();
        if(root.exists("Peers")) root.remove("Peers");

        libconfig::Setting& peers = root.add("Peers", libconfig::Setting::TypeArray);
        for(auto& it : peers_){
            NodeAddress address = it.second->GetAddress();
            peers.add(libconfig::Setting::TypeString) = address.ToString();
        }

        BlockChainConfiguration::SaveConfiguration();
    }

    uv_tcp_t* Server::GetHandle(){
        return &handle_;
    }

    NodeInfo Server::GetInfo(){
        LOCK_GUARD;
        return info_;
    }

    void Server::Start(){
        if(!IsStopped()) return;
        LoadNodeInformation();
        LoadPeers();
        if(pthread_create(&thread_, NULL, &NodeThread, NULL) != 0){
            std::stringstream ss;
            ss << "Couldn't start block chain server thread";
            CrashReport::GenerateAndExit(ss);
        }
    }

    bool Server::Shutdown(){
        if(!IsRunning()) return false;
        uv_async_send(&aterm_);

        int ret;
        if((ret = pthread_join(thread_, NULL)) != 0){
            std::stringstream ss;
            ss << "Couldn't join server thread: " << strerror(ret);
            CrashReport::GenerateAndExit(ss);
        }

        SetState(State::kStopped);
        return true;
    }

    bool Server::WaitForShutdown(){
        WaitForState(State::kStopped);
        return true;
    }

    bool Server::WaitForRunning(){
        WaitForState(State::kRunning);
        return true;
    }

    void Server::SetState(Server::State state){
        LOCK;
        state_ = state;
        SIGNAL_ALL;
    }

    void Server::WaitForState(Server::State state){
        LOCK;
        while(state_ != state) WAIT;
    }

    Server::State Server::GetState() {
        LOCK_GUARD;
        return state_;
    }

    uint32_t Server::GetNumberOfPeers(){
        LOCK_GUARD;
        uint32_t peers = peers_.size();
        return peers;
    }

    void Server::HandleTerminateCallback(uv_async_t* handle){
        uv_stop(handle->loop);
    }

    void* Server::NodeThread(void* ptr){
        LOG(INFO) << "starting server...";
        SetState(State::kStarting);
        uv_loop_t* loop = uv_loop_new();
        uv_async_init(loop, &aterm_, &HandleTerminateCallback);

        struct sockaddr_in addr;
        uv_ip4_addr("0.0.0.0", FLAGS_port, &addr);
        uv_tcp_init(loop, GetHandle());
        uv_tcp_keepalive(GetHandle(), 1, 60);
        int err;
        if((err = uv_tcp_bind(GetHandle(), (const struct sockaddr*)&addr, 0)) != 0){
            LOG(ERROR) << ""; //TODO:
            pthread_exit(0);
        }

        if((err = uv_listen((uv_stream_t*)GetHandle(), 100, &OnNewConnection)) != 0){
            LOG(ERROR) << ""; //TODO:
            pthread_exit(0);
        }

#ifdef TOKEN_DEBUG
        LOG(INFO) << "server " << GetID() << " listening @" << FLAGS_port;
#else
        LOG(INFO) << "server listening @" << FLAGS_port;
#endif//TOKEN_DEBUG
        SetState(State::kRunning);
        uv_run(loop, UV_RUN_DEFAULT);

        uv_loop_close(loop);
        pthread_exit(0);
    }

    void Server::OnNewConnection(uv_stream_t* stream, int status){
        if(status != 0){
            LOG(ERROR) << "connection error: " << uv_strerror(status);
            return;
        }

        NodeSession* session = new NodeSession();
        uv_tcp_init(stream->loop, (uv_tcp_t*)session->GetStream());
        LOG(INFO) << "client is connecting...";
        if((status = uv_accept(stream, (uv_stream_t*)session->GetStream())) != 0){
            LOG(ERROR) << "client accept error: " << uv_strerror(status);
            return;
        }

        LOG(INFO) << "client connected";
        if((status = uv_read_start(session->GetStream(), &Session::AllocBuffer, &OnMessageReceived)) != 0){
            LOG(ERROR) << "client read error: " << uv_strerror(status);
            return;
        }
    }

    void Server::OnMessageReceived(uv_stream_t* stream, ssize_t nread, const uv_buf_t* buff){
        NodeSession* session = (NodeSession*)stream->data;
        if(nread == UV_EOF){
            LOG(ERROR) << "client disconnected!";
            return;
        } else if(nread < 0){
            LOG(ERROR) << "[" << nread << "] client read error: " << std::string(uv_strerror(nread));
            return;
        } else if(nread == 0){
            LOG(WARNING) << "zero message size received";
            return;
        } else if(nread >= 4096){
            LOG(ERROR) << "too large of a buffer";
            return;
        }

        uint32_t offset = 0;

        std::vector<Handle<Message>> messages;
        do{
            uint32_t mtype = 0;
            memcpy(&mtype, &buff->base[offset + Message::kTypeOffset], Message::kTypeLength);

            uint64_t msize = 0;
            memcpy(&msize, &buff->base[offset + Message::kSizeOffset], Message::kSizeLength);

            //TODO: handle decode failures
            Handle<Message> msg = Message::Decode(static_cast<Message::MessageType>(mtype), msize, (uint8_t*)&buff->base[offset + Message::kDataOffset]);

            messages.push_back(msg);
            offset += (msize + Message::kHeaderSize);
        } while((offset + Message::kHeaderSize) < nread);

        for(size_t idx = 0; idx < messages.size(); idx++){
            Message* msg = messages[idx];
            HandleMessageTask task(session, msg);
            switch(msg->GetMessageType()){
#define DEFINE_HANDLER_CASE(Name) \
            case Message::k##Name##MessageType: \
                session->Handle##Name##Message(&task); \
                break;
                FOR_EACH_MESSAGE_TYPE(DEFINE_HANDLER_CASE);
#undef DEFINE_HANDLER_CASE
                case Message::kUnknownMessageType:
                default: //TODO: handle properly
                    break;
            }
        }
    }

    bool Server::BroadcastMessage(const Handle<Message>& msg){
        LOCK_GUARD;
        for(auto& it : peers_) it.second->Send(msg);
        return true;
    }

    void Server::RegisterPeer(const std::string& node_id, PeerSession* peer){
        LOCK_GUARD;
        peers_.insert({ node_id, peer });
        SavePeers();
    }

    void Server::UnregisterPeer(const std::string& node_id){
        LOCK_GUARD;
        peers_.erase(node_id);
    }

    void Server::GetPeers(std::vector<PeerInfo>& peers){
        LOCK_GUARD;
        for(auto& it : peers_) peers.push_back(PeerInfo(it.second));
    }

    bool Server::HasPeer(const std::string& node_id){
        LOCK_GUARD;
        return peers_.find(node_id) != peers_.end();
    }

    bool Server::HasPeer(const NodeAddress& address){
        LOCK_GUARD;
        for(auto& it : peers_){
            NodeAddress paddress = it.second->GetAddress();
            if(paddress == address) return true;
        }
        return false;
    }

    bool Server::ConnectTo(const NodeAddress &address){
        if(IsStarting() || IsSynchronizing()){
            WaitForState(State::kRunning);
            if(!IsRunning()) {
                LOG(WARNING) << "server is in " << GetState() << " state, not connecting!";
                return false;
            }
        }

        //-- Attention! --
        // this is not a memory leak, the memory will be freed upon
        // the session being closed
        return (new PeerSession(address))->Connect();
    }
}