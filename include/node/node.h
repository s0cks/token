#ifndef TOKEN_NODE_H
#define TOKEN_NODE_H

#include "message.h"
#include "session.h"

namespace Token{
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
        static void AfterHandleGetDataTask(uv_work_t* handle, int status);

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

        static inline bool
        BroadcastInventory(Block* block){
            std::vector<InventoryItem> items = {
                InventoryItem(block)
            };
            return Broadcast(InventoryMessage(items));
        }

        static inline bool
        BroadcastInventory(Transaction* tx){
            std::vector<InventoryItem> items = {
                InventoryItem(tx)
            };
            return Broadcast(InventoryMessage(items));
        }

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