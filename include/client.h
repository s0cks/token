#ifndef TOKEN_CLIENT_H
#define TOKEN_CLIENT_H

#include "session.h"
#include "command.h"

namespace Token{
    class ClientSession;
    class ClientSessionInfo : public SessionInfo{
        friend class ClientSession;
    protected:
        ClientSessionInfo(ClientSession* session);
    public:
        ~ClientSessionInfo(){}

        BlockHeader GetHead() const;
        NodeAddress GetPeerAddress() const;
        std::string GetPeerID() const;
        void operator=(const ClientSessionInfo& info);
    };

    class ClientSession : public Session{
        friend class ClientSessionInfo;
    private:
        uv_loop_t* loop_;
        uv_signal_t sigterm_;
        uv_signal_t sigint_;
        uv_tcp_t stream_;
        uv_timer_t hb_timer_;
        uv_timer_t hb_timeout_;
        uv_connect_t connection_;
        uv_pipe_t stdin_;
        uv_pipe_t stdout_;

        NodeAddress paddress_;
        std::string pid_;
        BlockHeader head_;

        void SetHead(const BlockHeader& head){
            head_ = head;
        }

        BlockHeader GetHead() const{
            return head_;
        }

        void SetPeerAddress(const NodeAddress& address){
            paddress_ = address;
        }

        NodeAddress GetPeerAddress() const{
            return paddress_;
        }

        void SetPeerID(const std::string& id){
            pid_ = id;
        }

        std::string GetPeerID() const{
            return pid_;
        }

        static void OnConnect(uv_connect_t* conn, int status);
        static void OnMessageReceived(uv_stream_t* stream, ssize_t nread, const uv_buf_t* buf);
        static void OnCommandReceived(uv_stream_t* stream, ssize_t nread, const uv_buf_t* buf);
        static void OnSignal(uv_signal_t* handle, int signum);
        static void OnHeartbeatTick(uv_timer_t* handle);
        static void OnHeartbeatTimeout(uv_timer_t* handle);

        virtual uv_stream_t* GetStream(){
            return (uv_stream_t*)&stream_;
        }

        inline uv_stream_t*
        GetStdinPipe(){
            return (uv_stream_t*)&stdin_;
        }
    public:
        ClientSession():
            stream_(),
            loop_(uv_loop_new()),
            sigterm_(),
            sigint_(),
            stdin_(),
            stdout_(),
            Session(&stream_){
            stdin_.data = this;
            stdout_.data = this;
            stream_.data = this;
            connection_.data = this;
            hb_timer_.data = this;
            hb_timeout_.data = this;
        }
        ~ClientSession(){}

        ClientSessionInfo GetInfo() const{
            return ClientSessionInfo(const_cast<ClientSession*>(this));
        }

#define DECLARE_MESSAGE_HANDLER(Name) \
    virtual void Handle##Name##Message(const Handle<HandleMessageTask>& task);
        FOR_EACH_MESSAGE_TYPE(DECLARE_MESSAGE_HANDLER)
#undef DECLARE_MESSAGE_HANDLER

#define DECLARE_COMMAND_HANDLER(Name, Text, Parameters) \
    void Handle##Name##Command(Name##Command* cmd);
        FOR_EACH_COMMAND(DECLARE_COMMAND_HANDLER);
#undef DECLARE_COMMAND_HANDLER

        void Connect(const NodeAddress& addr);
    };
}

#endif //TOKEN_CLIENT_H
