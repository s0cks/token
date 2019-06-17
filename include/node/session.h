#ifndef TOKEN_SESSION_H
#define TOKEN_SESSION_H

#include <uv.h>
#include <memory.h>
#include "bytes.h"
#include "message.h"

namespace Token{
#define FOR_EACH_SESSION_TYPE(V) \
    V(Peer) \
    V(Client)

#define FORWARD_DECLARE_SESSION(Name) \
    class Name##Session;
    FOR_EACH_SESSION_TYPE(FORWARD_DECLARE_SESSION)
#undef FORWARD_DECLARE_SESSION

#define DEFINE_SESSION(Name) \
    virtual const char* GetSessionTypeName() const{ return #Name; } \
    virtual Name##Session* As##Name##Session();

    class Session{
    public:
        enum class State{
            kConnected = 1,
            kDisconnected
        };

        static const size_t kReadBufferMax = 4096;
    protected:
        ByteBuffer read_buffer_;
        State state_;

        void OnMessageSent(uv_write_t* request, int status);
        virtual bool Handle(Message* msg) = 0;

        ByteBuffer*
        GetReadBuffer(){
            return &read_buffer_;
        }

        inline void
        Append(const uv_buf_t* buff){
            GetReadBuffer()->PutBytes((uint8_t*)buff->base, static_cast<uint32_t>(buff->len));
        }

        inline Message*
        GetNextMessage(){
            Message* msg = Message::Decode(GetReadBuffer());
            GetReadBuffer()->Clear();
            return msg;
        }

        Session():
            state_(State::kDisconnected),
            read_buffer_(kReadBufferMax){}
        friend class BlockChainServer;
    public:
        virtual ~Session(){}

        void InitializeBuffer(uv_buf_t* buff, size_t size){
            ByteBuffer* bb = GetReadBuffer();
            *buff = uv_buf_init((char*)bb->GetBytes(), static_cast<unsigned int>(size));
        }

        State GetState() const{
            return state_;
        }

        bool IsDisconnected() const{
            return GetState() == State::kDisconnected;
        }

        bool IsConnected() const{
            return GetState() == State::kConnected;
        }

        virtual uv_stream_t* GetStream() = 0;
        bool Send(Message* msg);

#define DECLARE_SESSION_TYPECHECK(Name) \
    bool Is##Name##Session(){ return As##Name##Session() != nullptr; } \
    virtual Name##Session* As##Name##Session(){ return nullptr; }
    FOR_EACH_SESSION_TYPE(DECLARE_SESSION_TYPECHECK)
#undef DECLARE_SESSION_TYPECHECK
    };

    class PeerSession : public Session{
    private:
        uv_loop_t* loop_;
        uv_tcp_t stream_;
        uv_connect_t connection_;
        uv_stream_t* handle_;
        struct sockaddr_in addr_;
        std::string address_;
        int port_;

        void Connect();
        bool Handle(Message* msg);
    public:
        PeerSession(uv_loop_t* loop, const std::string& address, int port):
            loop_(loop),
            address_(address),
            addr_(),
            port_(port),
            stream_(),
            handle_(nullptr),
            connection_(),
            Session(){
            Connect();
        }
        ~PeerSession(){}

        int GetPort() const{
            return port_;
        }

        std::string GetAddress() const{
            return address_;
        }

        uv_stream_t* GetStream(){
            return handle_;
        }

        void OnConnect(uv_connect_t* conn, int status);
        void OnMessageRead(uv_stream_t* stream, ssize_t nread, const uv_buf_t* buff);

        DEFINE_SESSION(Peer);

        friend std::ostream& operator<<(std::ostream& stream, const PeerSession& session){
            return stream << "Peer('" << session.GetAddress() << ":" << session.GetPort() << "')";
        }
    };

    class ClientSession : public Session{
    private:
        std::shared_ptr<uv_tcp_t> connection_;

        bool Handle(Message* msg);
    public:
        ClientSession(std::shared_ptr<uv_tcp_t> connection):
            connection_(connection){}
        ~ClientSession(){}

        uv_tcp_t*
        GetConnection(){
            return connection_.get();
        }

        uv_stream_t*
        GetStream(){
            return (uv_stream_t*)GetConnection();
        }

        DEFINE_SESSION(Client);
    };
}

#endif //TOKEN_SESSION_H