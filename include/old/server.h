#ifndef TOKEN_NODE_H
#define TOKEN_NODE_H

#include <pthread.h>
#include <string>
#include <vector>
#include <cstdint>
#include <uv.h>

#include "session.h"
#include "node/peer.h"

namespace Token{
    class ServerSession : public Session{
    private:
        uv_tcp_t handle_;

        ServerSession():
            handle_(){
            handle_.data = this;
        }
    protected:
        uv_stream_t* GetStream() const{
            return (uv_stream_t*)&handle_;
        }
    public:
        ~ServerSession(){
            //TODO: implement
        }
    };

    class Server{
    private:
        pthread_t thread_;

        static Server* GetInstance();
        static void* ServerThread(void* data);
        static void OnNewConnection(uv_stream_t* stream, int status);
        static void OnMessageReceived(uv_stream_t* stream, ssize_t nread, const uv_buf_t* buff);
        static void OnElection(uv_timer_t* handle);
        static void OnHeartbeat(uv_timer_t* handle);

        Server(): thread_(){}
    public:
        ~Server(){}

        bool Initialize(){
            return pthread_create(&thread_, NULL, &ServerThread, NULL) == 0;
        }

        uint32_t GetNumberOfPeers(){
            return 1; //TODO: fixme
        }
    };

    /*
    //TODO: pthread_equal for switching broadcast mechanism
    //https://thispointer.com/posix-how-to-get-thread-id-of-a-pthread-in-linux-pthread_self-pthread_equals/
    class BlockChainServer{
    public:


        static const uint32_t kBeatDelta = 8000;
        static const uint32_t kBeatSensitivity = 3;
    private:
        pthread_t thread_;
        pthread_rwlock_t rwlock_;
        ClusterMemberState state_;

        BlockChainServer();

        static BlockChainServer* GetInstance();

        static uint256_t GetNodeID(){
            return GetInstance()->node_id_;
        }

        static void IncrementVoteCounter();
        static void IncrementBeatCounter();
        static void IncrementLeaderTTL();
        static void SetVoteCounter(int count);
        static void SetBeatCounter(int count);
        static void SetLeaderTTL(int ttl);
        static void SetLeaderID(const uint256_t& id);
        static void SetState(ClusterMemberState state);
        static void SetLastHeartbeat(uint32_t ts);
        static int GetVoteCounter();
        static int GetBeatCounter();
        static int GetLeaderTTL();
        static uint32_t GetLastHeartbeat();
        static uint256_t GetLeaderID();
        static ClusterMemberState GetState();

        static void AllocBuffer(uv_handle_t* handle, size_t suggested_size, uv_buf_t* buff);

        static void* ServerThread(void* data);

        static void HandleAsyncBroadcastMessage(uv_async_t* handle);
        static void HandleBroadcastMessage(uv_work_t* req);
        static void HandleResolveInventory(uv_work_t* req);

#define DECLARE_HANDLE(Name) \
    static void Handle##Name##Message(uv_work_t* req);
        FOR_EACH_MESSAGE_TYPE(DECLARE_HANDLE);
#undef DECLARE_HANDLE
        static void AfterHandleMessage(uv_work_t* req, int status);
        static void AfterBroadcastMessage(uv_work_t* req, int status);
        static void AfterResolveInventory(uv_work_t* req, int status);

        friend class ClientSession;
    public:
        ~BlockChainServer(){}

        static bool StartServer();

        static bool IsConnectedTo(const std::string& address){
            auto pos = GetInstance()->peers_.find(address);
            return pos != GetInstance()->peers_.end();
        }

        static inline bool IsConnectedTo(const std::string& address, uint16_t port){
            std::stringstream stream;
            stream << address << ":" << port;
            return IsConnectedTo(stream.str());
        }

        static void WaitForShutdown();
        static void ConnectToPeer(const std::string& address, uint16_t port);
        static void AsyncBroadcastMessage(Message* msg);
        static void BroadcastMessage(Message* msg);

        template<typename MessageType>
        static inline void BroadcastMessage(const MessageType& msg){
            return BroadcastMessage((Message*) new MessageType(msg));
        }
    };
     */
}

#endif //TOKEN_SERVER_H