#ifndef TOKEN_SESSION_H
#define TOKEN_SESSION_H

#include <uv.h>
#include <iostream>
#include <memory>
#include <bitset>
#include "message.h"

namespace Token {
    class PeerSession{
        /**
         * Server Side
         *
         * Handles incoming blocks & routing
         */
    public:
        enum class State{
            kDisconnected = 0,
            kConnecting,
            kAuthenticating,
            kConnected,
        };
    private:
        uv_loop_t* loop_;
        std::shared_ptr<uv_tcp_t> conn_;
        State state_;

        inline uv_tcp_t*
        GetConnection() const{
            return conn_.get();
        }

        inline void
        SetState(State state){
            state_ = state;
        }

        bool AcceptsIdentity(Node::Messages::PeerIdentity* ident);
        void OnMessageSent(uv_write_t* req, int status);
        void SendIdentity();

        friend class BlockChainServer;
    public:
        PeerSession(uv_loop_t* loop, std::shared_ptr<uv_tcp_t> conn):
            loop_(loop),
            conn_(conn),
            state_(State::kConnecting){}
        ~PeerSession(){}

        State GetState() const{
            return state_;
        }

        bool IsDisconnected() const{
            return GetState() == State::kDisconnected;
        }

        bool IsConnecting() const{
            return GetState() == State::kConnecting ||
                    GetState() == State::kAuthenticating;
        }

        bool IsConnected() const{
            return GetState() == State::kConnected;
        }

        bool Send(Message* msg);
        bool Handle(uv_stream_t* stream, Token::Message* msg);
    };
}

#endif //TOKEN_SESSION_H
