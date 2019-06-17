#ifndef TOKEN_NODE_H
#define TOKEN_NODE_H

#include <uv.h>
#include <memory.h>
#include <map>
#include "bytes.h"
#include "session.h"
#include "message.h"
#include "blockchain.h"

namespace Token{
    class BlockChainServer{
    public:
        enum class State{
            kRunning = 1,
            kStopped
        };

        static const size_t MAX_BUFFER_SIZE = 4096;
    private:
        typedef std::map<uv_stream_t*, Session*> SessionMap;

        uv_loop_t loop_;
        uv_tcp_t server_;
        ByteBuffer read_buffer_;
        State state_;
        SessionMap sessions_;

        void OnNewConnection(uv_stream_t* server, int status);
        void OnMessageRead(uv_stream_t* stream, ssize_t nread, const uv_buf_t* buff);
        void OnMessageSent(uv_write_t* stream, int status);
        void Send(uv_stream_t* conn, Message* msg);
        void RemoveClient(uv_stream_t* stream);

        bool Handle(Session* session, Message* msg);

        BlockChainServer():
            sessions_(),
            state_(State::kStopped),
            server_(),
            loop_(),
            read_buffer_(MAX_BUFFER_SIZE){
        }
    public:
        ~BlockChainServer(){}

        static BlockChainServer* GetInstance();
        static int Initialize(int port);

        void Start();
        void Broadcast(uv_stream_t* from, Message* msg);
        bool ConnectTo(const std::string& address, int port);

        ByteBuffer* GetReadBuffer(){
            return &read_buffer_;
        }

        State GetState() const{
            return state_;
        }

        bool IsStopped() const{
            return GetState() == State::kStopped;
        }

        bool IsRunning() const{
            return GetState() == State::kRunning;
        }

        int GetNumberOfPeers() const{
            return sessions_.size();
        }
    };
}

#endif //TOKEN_NODE_H