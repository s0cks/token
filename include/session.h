#ifndef TOKEN_SESSION_H
#define TOKEN_SESSION_H

#include <uv.h>
#include <iostream>
#include <memory>
#include "message.h"

namespace Token {
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
            kDisconnected
        };

        static const size_t kReadBufferMax = 4096;
    protected:
        ByteBuffer read_buffer_;
        State state_;

        void OnMessageSent(uv_write_t *req, int status);

        virtual bool Handle(Message *msg) = 0;

        ByteBuffer *
        GetReadBuffer() {
            return &read_buffer_;
        }

        inline void
        Append(const uv_buf_t *buff) {
            GetReadBuffer()->PutBytes((uint8_t *) buff->base, static_cast<uint32_t>(buff->len));
        }

        inline Message *
        GetNextMessage() {
            uint32_t type = GetReadBuffer()->GetInt();
            Message *msg = Message::Decode(type, GetReadBuffer());
            GetReadBuffer()->Clear();
            return msg;
        }

        friend class BlockChain;

        Session() :
                state_(State::kDisconnected),
                read_buffer_(kReadBufferMax) {}
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

        bool Send(Message *msg);

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

        bool Handle(Message *msg);

        void Connect();

        void OnConnect(uv_connect_t *conn, int status);

        void OnMessageRead(uv_stream_t *stream, ssize_t nread, const uv_buf_t *buff);

    public:
        PeerSession(uv_loop_t *loop, std::string addr, int port);

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

        DECLARE_SESSION(Peer);
    };

    class ClientSession : public Session {
    private:
        std::shared_ptr<uv_tcp_t> connection_;

        bool Handle(Message *msg);

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