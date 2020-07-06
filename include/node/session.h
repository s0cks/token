#ifndef TOKEN_SESSION_H
#define TOKEN_SESSION_H

#include <mutex>
#include <condition_variable>
#include "message.h"
#include "node_info.h"

namespace Token{
    //TODO:
    // - create + use Bytes class
    // - scope?
    class Session{
    public:
        static const size_t kBufferSize = 4096;

        enum State{
            kDisconnected = 0,
            kConnecting,
            kConnected
        };
    protected:
        static void AllocBuffer(uv_handle_t* handle, size_t suggested_size, uv_buf_t* buff);
        static void OnMessageSent(uv_write_t* req, int status);

        std::recursive_mutex mutex_;
        std::condition_variable_any cond_;
        State state_;

        Session():
            mutex_(),
            cond_(),
            state_(kDisconnected){}

        virtual uv_stream_t* GetStream() = 0;
        void SetState(State state);

        friend class Node;
    public:
        State GetState();

        void Send(Message* msg);
        void Send(std::vector<Message*>& messages);

        bool IsDisconnected(){
            return GetState() == kDisconnected;
        }

        bool IsConnecting(){
            return GetState() == kConnecting;
        }

        bool IsConnected(){
            return GetState() == kConnected;
        }

        void WaitForState(State state);
    };

    /*
     * Preserve these
        bool IsResolved(const InventoryItem& item){
            uint256_t hash = item.GetHash();
            if(item.IsTransaction()){
                Transaction* tx;
                if((tx = TransactionPool::GetTransaction(hash))){
                    return true;
                }
            } else if(item.IsBlock()){
                Block* blk;
                if((blk = BlockChain::GetBlockData(hash))){
                    return true;
                }

                if((blk = BlockPool::GetBlock(hash))){
                    return true;
                }
            }
            return false;
        }

        inline void
        WaitForTransaction(const uint256_t& hash){
            WaitForItem(InventoryItem(InventoryItem::kTransaction, hash));
        }

        inline void
        WaitForBlock(const uint256_t& hash){
            WaitForItem(InventoryItem(InventoryItem::kBlock, hash));
        }

        void WaitForItem(const InventoryItem& item){
            pthread_mutex_trylock(&rmutex_);
            LOG(INFO) << "waiting for: " << item.GetHash();
            while(!IsResolved(item)) pthread_cond_wait(&rcond_, &rmutex_);
            pthread_mutex_unlock(&rmutex_);
        }

        void WaitForItems(std::vector<InventoryItem> items){
            if(items.empty()) return;
            InventoryItem next = items.back();
            items.pop_back();
            WaitForItem(next);
            return WaitForItems(items);
        }

        void OnHash(const uint256_t& hash){
            pthread_mutex_trylock(&rmutex_);
            pthread_cond_signal(&rcond_);
            pthread_mutex_unlock(&rmutex_);
        }
    */

    class NodeSession : public Session{
    private:
        NodeInfo info_;
        uv_tcp_t handle_;
        uv_timer_t heartbeat_;

        NodeSession():
            Session(),
            handle_(),
            info_(&handle_){
            handle_.data = this;
            heartbeat_.data = this;
        }

        virtual uv_stream_t* GetStream(){
            return (uv_stream_t*)&handle_;
        }

        friend class Node;
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
    };

    class HandleMessageTask;
    class PeerSession : public Session{
    private:
        pthread_t thread_;
        uv_connect_t conn_;
        uv_tcp_t socket_;
        uv_async_t shutdown_;
        uv_timer_t heartbeat_;
        std::string node_id_;
        NodeAddress node_addr_;

        static void* PeerSessionThread(void* data);
        static void OnShutdown(uv_async_t* handle);
        static void OnConnect(uv_connect_t* conn, int status);
        static void OnMessageReceived(uv_stream_t* stream, ssize_t nread, const uv_buf_t* buf);

#define DECLARE_MESSAGE_HANDLER(Name) \
    static void Handle##Name##Message(HandleMessageTask* task);
    FOR_EACH_MESSAGE_TYPE(DECLARE_MESSAGE_HANDLER);

        static void HandleSynchronizeBlocksTask(uv_work_t* handle);
        static void AfterSynchronizeBlocksTask(uv_work_t* handle, int status);
    protected:
        virtual uv_stream_t* GetStream(){
            return conn_.handle;
        }

        uv_loop_t* GetLoop() const{
            return socket_.loop;
        }
    public:
        PeerSession(const NodeAddress& addr):
                Session(),
                thread_(),
                socket_(),
                conn_(),
                node_addr_(addr),
                node_id_(){
            socket_.data = this;
            conn_.data = this;
        }
        ~PeerSession(){}

        NodeInfo GetInfo() const{
            return NodeInfo(node_id_, node_addr_);
        }

        NodeAddress GetAddress() const{
            return node_addr_;
        }

        std::string GetID() const{
            return node_id_;
        }

        void Shutdown();
        bool Connect();
    };

    class NodeClient : public Session{
    private:
        uv_loop_t* loop_;
        uv_signal_t sigterm_;
        uv_signal_t sigint_;
        uv_tcp_t stream_;
        uv_connect_t connection_;
        uv_pipe_t stdin_;
        uv_pipe_t stdout_;

        static void OnConnect(uv_connect_t* conn, int status);
        static void OnMessageReceived(uv_stream_t* stream, ssize_t nread, const uv_buf_t* buf);
        static void OnCommandReceived(uv_stream_t* stream, ssize_t nread, const uv_buf_t* buf);
        static void OnSignal(uv_signal_t* handle, int signum);

#define DECLARE_MESSAGE_HANDLER(Name) \
    static void Handle##Name##Message(HandleMessageTask* task);
        FOR_EACH_MESSAGE_TYPE(DECLARE_MESSAGE_HANDLER);
#undef DECLARE_MESSAGE_HANDLER

        virtual uv_stream_t* GetStream(){
            return (uv_stream_t*)&stream_;
        }

        inline uv_stream_t*
        GetStdinPipe(){
            return (uv_stream_t*)&stdin_;
        }
    public:
        NodeClient():
                Session(),
                loop_(uv_loop_new()),
                sigterm_(),
                sigint_(),
                stream_(),
                stdin_(),
                stdout_(){
            stdin_.data = this;
            stdout_.data = this;
            stream_.data = this;
            connection_.data = this;
        }
        ~NodeClient(){}

        void Connect(const NodeAddress& addr);
    };
}

#endif //TOKEN_SESSION_H