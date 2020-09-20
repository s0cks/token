#ifndef TOKEN_CLIENT_H
#define TOKEN_CLIENT_H

#include "session.h"
#include "command.h"

namespace Token{
#define FOR_EACH_CLIENT_COMMAND(V) \
    V(Status, ".status", 0) \
    V(Disconnect, ".disconnect", 0) \
    V(Transaction, "tx", 3)

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

    class ClientCommand;
    class ClientCommandHandler;
    class ClientSession : public Session{
        //TODO: add client command handler
        friend class ClientSessionInfo;
        friend class ClientCommandHandler;
    private:
        uv_signal_t sigterm_;
        uv_signal_t sigint_;
        uv_tcp_t stream_;
        uv_timer_t hb_timer_;
        uv_timer_t hb_timeout_;
        uv_connect_t connection_;
        ClientCommandHandler* handler_;

        // Info
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
        static void OnSignal(uv_signal_t* handle, int signum);
        static void OnHeartbeatTick(uv_timer_t* handle);
        static void OnHeartbeatTimeout(uv_timer_t* handle);

        virtual uv_loop_t* GetLoop(){
            return stream_.loop;
        }

        virtual uv_stream_t* GetStream(){
            return (uv_stream_t*)&stream_;
        }
    public:
        ClientSession(bool use_stdin);
        ~ClientSession(){}

        ClientSessionInfo GetInfo() const{
            return ClientSessionInfo(const_cast<ClientSession*>(this));
        }

#define DECLARE_MESSAGE_HANDLER(Name) \
    virtual void Handle##Name##Message(const Handle<HandleMessageTask>& task);
        FOR_EACH_MESSAGE_TYPE(DECLARE_MESSAGE_HANDLER)
#undef DECLARE_MESSAGE_HANDLER

        void Connect(const NodeAddress& addr);
        void Disconnect();
    };

    class ClientCommand : public Command{
    public:
        ClientCommand(std::deque<std::string>& args): Command(args){}
        ~ClientCommand() = default;

#define DEFINE_TYPE_CHECK(Name, Text, ArgumentCount) \
        bool Is##Name##Command() const{ return !strncmp(name_.data(), (Text), strlen((Text))); }
        FOR_EACH_CLIENT_COMMAND(DEFINE_TYPE_CHECK)
#undef DEFINE_TYPE_CHECK
    };

    class ClientCommandHandler{
        friend class ClientSession;
    private:
        ClientSession* session_;
        uv_pipe_t stdin_;

        ClientCommandHandler(ClientSession* session):
            session_(session),
            stdin_(){
            stdin_.data = this;
        }

        inline void
        Send(const Handle<Message>& msg){
            GetSession()->Send(msg);
        }

        inline void
        Send(std::vector<Handle<Message>>& messages){
            GetSession()->Send(messages);
        }

#define DEFINE_SEND(Name) \
        inline void \
        Send(const Handle<Name##Message>& msg){ \
            Send(msg.CastTo<Message>()); \
        }
        FOR_EACH_MESSAGE_TYPE(DEFINE_SEND)
#undef DEFINE_SEND

        static void OnCommandReceived(uv_stream_t* stream, ssize_t nread, const uv_buf_t* buf);
    public:
        ~ClientCommandHandler() = default;

        ClientSession* GetSession() const{
            return session_;
        }

#define DECLARE_COMMAND_HANDLER(Name, Text, Parameters) \
        void Handle##Name##Command(ClientCommand* cmd);
        FOR_EACH_CLIENT_COMMAND(DECLARE_COMMAND_HANDLER);
#undef DECLARE_COMMAND_HANDLER

        void Start();
    };
}

#endif //TOKEN_CLIENT_H
