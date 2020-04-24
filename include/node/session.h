#ifndef TOKEN_SESSION_H
#define TOKEN_SESSION_H

#include <uv.h>
#include <pthread.h>
#include <string>
#include <cstdint>

#include "message.h"
#include "buffer.h"
#include "block_chain.h"

namespace Token{
    enum class SessionState{
        kConnecting,
        kHandshaking,
        kSynchronizing,
        kConnected,
        kDisconnected
    };

    //TODO: establish timer for ping
    class Session{
    private:
        SessionState state_;
        ByteBuffer rbuffer_;
        ByteBuffer wbuffer_;

        friend class BlockChainServer;
        friend class ClientSession;
    protected:
        void SetState(SessionState state){
            state_ = state;
        }

        ByteBuffer* GetReadBuffer(){
            return &rbuffer_;
        }

        ByteBuffer* GetWriteBuffer(){
            return &wbuffer_;
        }

        static void OnMessageSent(uv_write_t* req, int status);
        static void AllocBuffer(uv_handle_t* handle, size_t suggested_size, uv_buf_t* buff);

        Session();
    public:
        virtual ~Session(){}
        virtual void Send(Token::Message* msg);
        virtual uv_stream_t* GetStream() const = 0;

        void SendPing(const std::string& nonce=GenerateNonce()){
            LOG(INFO) << "ping: " << nonce;
            PingMessage msg(nonce);
            Send(&msg);
        }

        void SendPong(const std::string& nonce=GenerateNonce()){
            PongMessage msg(nonce);
            Send(&msg);
        }

        void SendBlock(const Block& block){
            BlockMessage msg(block);
            Send(&msg);
        }

        void SendGetData(const std::string& hash){
            GetDataMessage msg(hash);
            Send(&msg);
        }

        void SendGetBlocks(const std::string& first, const std::string& last){
            GetBlocksMessage msg(first, last);
            Send(&msg);
        }

        void SendTransaction(const Transaction& tx){
            TransactionMessage msg(tx);
            Send(&msg);
        }

        void SendVersion(const std::string& nonce=GenerateNonce()){
            VersionMessage msg(nonce);
            Send(&msg);
        }

        void SendVerack(const std::string& nonce=GenerateNonce()){
            VerackMessage msg(nonce);
            Send(&msg);
        }

        void SendInventory(const Proto::BlockChainServer::Inventory& inventory){
            InventoryMessage msg(inventory);
            Send(&msg);
        }

        SessionState GetState() const{
            return state_;
        }

        bool IsConnecting() const{
            return GetState() == SessionState::kConnecting;
        }

        bool IsConnected() const{
            return GetState() == SessionState::kConnected;
        }

        bool IsDisconnected() const{
            return GetState() == SessionState::kDisconnected;
        }

        bool IsSynchronizing() const{
            return GetState() == SessionState::kSynchronizing;
        }

        bool IsHandshaking() const{
            return GetState() == SessionState::kHandshaking;
        }

        virtual bool IsPeerSession() const{
            return false;
        }

        virtual bool IsClientSession() const{
            return false;
        }
    };

    class PeerSession : public Session{
    private:
        friend class BlockChainServer;

        uv_tcp_t handle_;
        uv_timer_t ping_timer_;
        uv_timer_t timeout_timer_;

        PeerSession():
            handle_(),
            ping_timer_(),
            timeout_timer_(),
            Session(){
            handle_.data = this;
            ping_timer_.data = this;
            timeout_timer_.data = this;
        }
    public:
        ~PeerSession(){}

        uv_tcp_t* GetHandle(){
            return &handle_;
        }

        virtual uv_stream_t* GetStream() const{
            return (uv_stream_t*)&handle_;
        }

        bool IsPeerSession() const{
            return true;
        }
    };

    class ClientSession : public Session{
    private:
        pthread_t thread_;
        std::string address_;
        uint16_t port_;
        uv_tcp_t stream_;
        uv_connect_t conn_;
        uv_stream_t* handle_;

        static void ResolveInventory(uv_work_t* req);
        static void HandleInventoryMessage(uv_work_t* req);
        static void HandleVersionMessage(uv_work_t* req);
        static void HandleVerackMessage(uv_work_t* req);
        static void HandleBlockMessage(uv_work_t* req);
        static void HandlePingMessage(uv_work_t* req);
        static void AfterResolveInventory(uv_work_t* req, int status);
        static void AfterProcessMessage(uv_work_t* req, int status);

        static void OnConnect(uv_connect_t* conn, int status);
        static void OnMessageReceived(uv_stream_t* stream, ssize_t nread, const uv_buf_t* buff);
        static void* ClientSessionThread(void* data);
    public:
        ClientSession(const std::string& address, uint16_t port):
            address_(address),
            port_(port),
            stream_(),
            conn_(),
            handle_(nullptr){}
        ~ClientSession(){}

        const char* GetAddress(){
            return address_.c_str();
        }

        uint16_t GetPort(){
            return port_;
        }

        uv_stream_t* GetStream() const{
            return handle_;
        }

        bool IsClientSession() const{
            return true;
        }

        int Connect();
    };
}

#endif //TOKEN_SESSION_H