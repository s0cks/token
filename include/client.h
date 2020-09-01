#ifndef TOKEN_CLIENT_H
#define TOKEN_CLIENT_H

#include "session.h"

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

        Handle<Block> GetHead() const;
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

        // Info
        NodeAddress paddress_;
        std::string pid_;
        Handle<Block> head_;

        void SetHead(const Handle<Block>& head){
            head_ = head;
        }

        Handle<Block> GetHead() const{
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
        ClientSession(bool use_stdin):
            stream_(),
            sigterm_(),
            sigint_(),
            Session(&stream_){
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

        void Connect(const NodeAddress& addr);
        void Disconnect();
    };

    class ClientCommand{
        friend class ClientSession;
        friend class ClientCommandHandler;
    public:
        enum class Type{
            kUnknownType=0,
#define DECLARE_TYPE(Name, Text, Parameters) k##Name##Type,
            FOR_EACH_CLIENT_COMMAND(DECLARE_TYPE)
#undef DECLARE_TYPE
        };
    private:
        Type type_;
        std::deque<std::string> args_;

        ClientCommand(Type type, std::deque<std::string>& args):
            type_(type),
            args_(args){}
        ClientCommand(const std::string& name, std::deque<std::string>& args):
            type_(GetCommand(name)),
            args_(args){}
        ClientCommand(std::deque<std::string>& args):
            type_(GetCommand(args)),
            args_(args){}

        inline Type
        GetType() const{
            return type_;
        }

        static inline Type
        GetCommand(const std::string& name){
#define DECLARE_CHECK(Name, Text, Parameters) \
            if(!strncmp(name.c_str(), (Text), strlen(Text))){ \
                return Type::k##Name##Type; \
            }
            FOR_EACH_CLIENT_COMMAND(DECLARE_CHECK)
#undef DECLARE_CHECK
            return Type::kUnknownType;
        }

        static inline Type
        GetCommand(std::deque<std::string>& args){
            std::string name = args.front();
            args.pop_front();
            return GetCommand(name);
        }
    public:
        ~ClientCommand() = default;

        std::string GetCommandName() const;

#define DECLARE_CHECK(Name, Text, Parameters) \
        bool Is##Name##Command() const{ return GetType() == ClientCommand::Type::k##Name##Type; }
        FOR_EACH_CLIENT_COMMAND(DECLARE_CHECK)
#undef DECLARE_CHECK

        bool HasNextArgument() const{
            return !args_.empty();
        }

        std::string GetNextArgument(){
            std::string arg = args_.front();
            args_.pop_front();
            return arg;
        }

        uint256_t GetNextArgumentHash(){
            return HashFromHexString(GetNextArgument());
        }

        uint32_t GetNextArgumentUInt32(){
            std::string arg = GetNextArgument();
            return (uint32_t)atol(arg.c_str());
        }

        friend std::ostream& operator<<(std::ostream& stream, const ClientCommand& cmd){
            std::vector<std::string> args(cmd.args_.begin(), cmd.args_.end());
            stream << cmd.GetCommandName();
            stream << "(";
            for(size_t idx = 0; idx < args.size(); idx++){
                stream << args[idx];
                if(idx < (args.size() - 1)) stream << ",";
            }
            stream << ")";
            return stream;
        }
    };

    class ClientCommandHandler{
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
