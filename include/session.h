#ifndef TOKEN_SESSION_H
#define TOKEN_SESSION_H

#include <uv.h>
#include <iostream>
#include <memory>
#include <bitset>
#include "message.h"

namespace Token {
    class BlockChainServer;

#define FOR_EACH_SESSION(V) \
    V(Peer) \
    V(Client)

#define FORWARD_DECLARE(Name) \
    class Name##Session;
    FOR_EACH_SESSION(FORWARD_DECLARE)
#undef FORWARD_DECLARE

#define DECLARE_SESSION(Name) \
    virtual const char* GetTypeName() const{ return #Name; } \
    virtual Name##Session* As##Name##Session();

    class Session {
    public:
        enum class State {
            kConnected,
            kHandshake,
            kDisconnected
        };

        static const size_t kReadBufferMax = 4096;
    protected:
        BlockChainServer* parent_;
        ByteBuffer* read_buffer_;
        State state_;

        void OnMessageSent(uv_write_t *req, int status);

        virtual bool Handle(uv_stream_t* stream, Message *msg) = 0;

        ByteBuffer *
        GetReadBuffer() {
            return read_buffer_;
        }

        inline void
        Append(const uv_buf_t* buff, ssize_t nread) {
            uint8_t bytes[nread + 1];
            memcpy(bytes, buff->base, nread);
            bytes[nread + 1] = '\0';
            GetReadBuffer()->PutBytes(bytes, nread + 1);
        }

        inline void
        ClearReadBuffer(){
            GetReadBuffer()->Clear();
        }

        Session(BlockChainServer* parent=nullptr):
            parent_(parent),
            state_(State::kDisconnected),
            read_buffer_(new ByteBuffer(kReadBufferMax)){}
        friend class BlockChainServer;
    public:
        ~Session() {}

        void InitializeBuffer(uv_buf_t *buff, size_t size) {
            ByteBuffer *bb = GetReadBuffer();
            *buff = uv_buf_init((char *) bb->GetBytes(), static_cast<unsigned int>(size));
        }

        State
        GetState() const {
            return state_;
        }

        bool
        IsConnected() const {
            return GetState() == State::kConnected;
        }

        virtual uv_stream_t *GetStream() = 0;

        bool Send(uv_stream_t* stream, Message *msg);

#define DECLARE_SESSION_TYPECHECK(Name) \
    bool Is##Name##Session(){ return As##Name##Session() != nullptr; } \
    virtual Name##Session* As##Name##Session() { return nullptr; }
        FOR_EACH_SESSION(DECLARE_SESSION_TYPECHECK)
#undef DECLARE_SESSION_TYPECHECK
    };

    class PeerSession : public Session {
    private:
        uv_loop_t *loop_;
        uv_tcp_t stream_;
        uv_connect_t connection_;
        uv_stream_t *handle_;
        struct sockaddr_in address_;
        std::string addressstr_;
        int port_;
        pthread_t thread_;

        bool Handle(uv_stream_t* stream, Message *msg);
        void OnConnect(uv_connect_t *conn, int status);
        void OnMessageRead(uv_stream_t *stream, ssize_t nread, const uv_buf_t *buff);
    public:
        PeerSession(BlockChainServer* server, std::string addr, int port);
        ~PeerSession() {}

        int
        GetPort() const {
            return port_;
        }

        std::string
        GetAddress() const {
            return addressstr_;
        }

        uv_stream_t *
        GetStream() {
            return handle_;
        }

        friend std::ostream &operator<<(std::ostream &stream, const PeerSession &self) {
            stream << self.GetAddress() << ":" << self.GetPort();
            return stream;
        }

        bool Connect();
        void Disconnect();
        int ConnectToPeer();

        BlockChainServer* GetParent() const{
            return parent_;
        }

        DECLARE_SESSION(Peer);
    };


    class ClientSession : public Session {
    private:
        std::shared_ptr<uv_tcp_t> connection_;

        bool Handle(uv_stream_t* stream, Message *msg);

        friend class BlockChainServer;
    public:
        ClientSession(std::shared_ptr<uv_tcp_t> conn) :
                connection_(conn) {}
        ~ClientSession() {}

        uv_stream_t *
        GetStream() {
            return (uv_stream_t *) GetConnection();
        }

        uv_tcp_t *
        GetConnection() const {
            return connection_.get();
        }

        friend std::ostream &operator<<(std::ostream &stream, const ClientSession &self) {
            return stream;
        }

        DECLARE_SESSION(Client);
    };
}

#endif //TOKEN_SESSION_H
