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
    class Session{
    public:
        enum State{
            kConnecting,
            kConnected,
            kDisconnected
        };

        static const intptr_t kBufferInitSize = 4096;
    private:
        pthread_rwlock_t rwlock_;
        State state_;
        ByteBuffer rbuff_;
        ByteBuffer wbuff_;
    protected:
        void SetState(State state){
            pthread_rwlock_wrlock(&rwlock_);
            state_ = state;
            pthread_rwlock_unlock(&rwlock_);
        }

        virtual uv_stream_t* GetStream() const = 0;

        Session():
            state_(kDisconnected),
            rbuff_(kBufferInitSize),
            wbuff_(kBufferInitSize){}
    public:
        virtual ~Session() = default;

        State GetState(){
            pthread_rwlock_rdlock(&rwlock_);
            State state = state_;
            pthread_rwlock_unlock(&rwlock_);
            return state;
        }

        bool IsConnected(){
            return GetState() == kConnected;
        }

        bool IsConnecting(){
            return GetState() == kConnecting;
        }

        bool IsDisconnected(){
            return GetState() == kDisconnected;
        }

        ByteBuffer* GetReadBuffer(){
            return &rbuff_;
        }

        ByteBuffer* GetWriteBuffer(){
            return &wbuff_;
        }

        void Send(Message* msg);
    };

    /*
    class ClientSession : public Session{
    private:
        pthread_t thread_;
        std::string address_;
        uint16_t port_;
        uv_tcp_t stream_;
        uv_connect_t conn_;
        uv_stream_t* handle_;

#define DECLARE_HANDLE(Name) \
    static void Handle##Name##Message(uv_work_t* req);

        FOR_EACH_MESSAGE_TYPE(DECLARE_HANDLE);
        static void AfterHandleMessage(uv_work_t* req, int status);

        static void ResolveInventory(uv_work_t* req);
        static void AfterResolveInventory(uv_work_t* req, int status);

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

     */
}

#endif //TOKEN_SESSION_H