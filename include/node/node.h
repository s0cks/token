#ifndef TOKEN_NODE_H
#define TOKEN_NODE_H

#include "common.h"
#include "message.h"
#include "peer.h"
#include "session.h"

namespace Token{
    class NodeSession : public Session{
    public:
        enum State{
            kConnecting,
            kConnected,
            kDisconnected,
        };

        friend class Node;
    private:
        pthread_rwlock_t rwlock_;
        State state_;
        uv_tcp_t handle_;
        NodeInfo info_;

        pthread_mutex_t rmutex_;
        pthread_cond_t rcond_;

        NodeSession():
            state_(kDisconnected),
            handle_(),
            info_(&handle_),
            rmutex_(),
            rcond_(){
            pthread_mutex_init(&rmutex_, NULL);
            pthread_cond_init(&rcond_, NULL);

            handle_.data = this;
            SetState(kConnecting);
        }

        uv_stream_t* GetHandle(){
            return (uv_stream_t*)&handle_;
        }

        bool IsResolved(const uint256_t& hash){
            //TODO: freeme
            Block* block = nullptr;
            if((block = BlockChain::GetBlockData(hash))) return true;
            if((block = BlockPool::GetBlock(hash))) return true;
            return false;
        }

        void WaitForHash(const uint256_t& hash){
            pthread_mutex_lock(&rmutex_);
            while(!IsResolved(hash)) pthread_cond_wait(&rcond_, &rmutex_);
            pthread_mutex_unlock(&rmutex_);
        }

        void OnHash(const uint256_t& hash){
            pthread_mutex_trylock(&rmutex_);
            pthread_cond_signal(&rcond_);
            pthread_mutex_unlock(&rmutex_);
        }

        static void OnMessageSent(uv_write_t* req, int status){
            if(status != 0) LOG(ERROR) << "failed to send message: " << uv_strerror(status);
            if(req) free(req);
        }
    public:
        ~NodeSession(){}

        NodeInfo GetInfo() const{
            return info_;
        }

        std::string GetNodeID() const{
            return GetInfo().GetNodeID();
        }

        NodeAddress GetAddress() const{
            return GetInfo().GetNodeAddress();
        }

        State GetState(){
            uv_rwlock_rdlock(&rwlock_);
            State state = state_;
            pthread_rwlock_unlock(&rwlock_);
            return state;
        }

        bool IsConnecting(){
            return GetState() == kConnecting;
        }

        bool IsConnected(){
            return GetState() == kConnected;
        }

        bool IsDisconnected(){
            return GetState() == kDisconnected;
        }

        void SetState(State state){
            pthread_rwlock_trywrlock(&rwlock_);
            state_ = state;
            pthread_rwlock_unlock(&rwlock_);
        }

        void Send(const Message& msg){
            LOG(INFO) << "sending " << msg;

            uint32_t rtype = static_cast<uint32_t>(msg.GetType());
            uint64_t rsize = msg.GetSize();
            uint64_t total_size = Message::kHeaderSize + rsize;

            uint8_t bytes[total_size];
            memcpy(&bytes[Message::kTypeOffset], &rtype, Message::kTypeLength);
            memcpy(&bytes[Message::kSizeOffset], &rsize, Message::kSizeLength);
            msg.Encode(&bytes[Message::kDataOffset], rsize);

            uv_buf_t buff = uv_buf_init((char*)bytes, total_size);
            uv_write_t* req = (uv_write_t*)malloc(sizeof(uv_write_t));
            req->data = this;
            uv_write(req, GetHandle(), &buff, 1, &OnMessageSent);
        }
    };

    class Task;
    class Node{
    private:
        pthread_t thid_;
        uv_tcp_t server_;
        NodeInfo info_;
        pthread_rwlock_t rwlock_;
        std::map<std::string, PeerSession*> peers_;

        static inline void
        RegisterPeer(const std::string& node_id, PeerSession* peer){
            Node* node = GetInstance();
            pthread_rwlock_trywrlock(&node->rwlock_);
            node->peers_.insert({ node_id, peer });
            pthread_rwlock_unlock(&node->rwlock_);
        }

        static inline void
        UnregisterPeer(const std::string& node_id){
            Node* node = GetInstance();
            pthread_rwlock_trywrlock(&node->rwlock_);
            node->peers_.erase(node_id);
            pthread_rwlock_unlock(&node->rwlock_);
        }

        static inline bool
        HasPeer(const std::string& node_id){
            Node* node = GetInstance();
            pthread_rwlock_tryrdlock(&node->rwlock_);
            bool found = node->peers_.find(node_id) != node->peers_.end();
            pthread_rwlock_unlock(&node->rwlock_);
            return found;
        }

        static void* NodeThread(void* ptr);
        static void AllocBuffer(uv_handle_t* handle, size_t suggested_size, uv_buf_t* buff);
        static void OnNewConnection(uv_stream_t* stream, int status);
        static void OnMessageReceived(uv_stream_t* stream, ssize_t nread, const uv_buf_t* buf);

#define DECLARE_HANDLER(Name) \
    static void Handle##Name##Message(uv_work_t* handle);
    FOR_EACH_MESSAGE_TYPE(DECLARE_HANDLER);
#undef DECLARE_HANDLER
        static void AfterHandleMessage(uv_work_t* handle, int status);

        static void HandleGetDataTask(uv_work_t* handle);
        static void AfterHandleGetData(uv_work_t* handle, int status);

        uv_tcp_t* GetHandle(){
            return &server_;
        }

        Node():
            thid_(),
            server_(),
            info_(&server_),
            rwlock_(){
            server_.data = this;
        }

        friend class BlockMiner;
        friend class PeerSession; //TODO: revoke access
    public:
        ~Node(){}

        static Node* GetInstance(){
            static Node node;
            return &node;
        }

        static bool Broadcast(const Message& msg);

        static bool ConnectTo(const NodeAddress& address){
            Node* node = GetInstance();
            //TODO: fixme
             PeerSession* peer = new PeerSession(address);
            return peer->Connect();
        }

        static bool ConnectTo(const std::string& address, uint32_t port){
            return ConnectTo(NodeAddress(address, port));
        }

        static bool Start(){
            Node* node = GetInstance();
            return pthread_create(&node->thid_, NULL, &NodeThread, node) == 0;
        }

        static bool WaitForShutdown(){
            Node* node = GetInstance();
            return pthread_join(node->thid_, NULL) == 0;
        }

        static uint32_t GetNumberOfPeers(){
            Node* node = GetInstance();
            pthread_rwlock_tryrdlock(&node->rwlock_);
            uint32_t peers = node->peers_.size();
            pthread_rwlock_unlock(&node->rwlock_);
            return peers;
        }

        static NodeInfo GetInfo(){
            Node* node = GetInstance();
            return node->info_;
        }
    };
}

#endif //TOKEN_NODE_H