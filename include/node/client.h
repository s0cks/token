#ifndef TOKEN_CLIENT_H
#define TOKEN_CLIENT_H

#include "session.h"
#include "command.h"

namespace Token{
    class NodeClient : public Session{
    private:
        uv_loop_t* loop_;
        uv_signal_t sigterm_;
        uv_signal_t sigint_;
        uv_tcp_t stream_;
        uv_connect_t connection_;
        uv_pipe_t stdin_;
        uv_pipe_t stdout_;
        NodeInfo peer_;

        static void OnConnect(uv_connect_t* conn, int status);
        static void OnMessageReceived(uv_stream_t* stream, ssize_t nread, const uv_buf_t* buf);
        static void OnCommandReceived(uv_stream_t* stream, ssize_t nread, const uv_buf_t* buf);
        static void OnSignal(uv_signal_t* handle, int signum);

        virtual uv_stream_t* GetStream(){
            return (uv_stream_t*)&stream_;
        }

        inline uv_stream_t*
        GetStdinPipe(){
            return (uv_stream_t*)&stdin_;
        }

        inline void
        SetPeer(const NodeInfo& info){
            peer_ = info;
        }
    public:
        NodeClient():
                peer_(),
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
        }
        ~NodeClient(){}

#define DECLARE_MESSAGE_HANDLER(Name) \
    virtual void Handle##Name##Message(HandleMessageTask* task);
        FOR_EACH_MESSAGE_TYPE(DECLARE_MESSAGE_HANDLER)
#undef DECLARE_MESSAGE_HANDLER

#define DECLARE_COMMAND_HANDLER(Name, Text, Parameters) \
    void Handle##Name##Command(Name##Command* cmd);
        FOR_EACH_COMMAND(DECLARE_COMMAND_HANDLER);
#undef DECLARE_COMMAND_HANDLER

        NodeInfo GetPeer() const{
            return peer_;
        }

        void Connect(const NodeAddress& addr);
    };
}

#endif //TOKEN_CLIENT_H
