#ifndef TOKEN_SHELL_H
#define TOKEN_SHELL_H

#include <uv.h>
#include "bytes.h"
#include "session.h"

namespace Token{
    class BlockChainServerShell{
    private:
        uv_loop_t* loop_;
        uv_pipe_t input_;
        PeerSession* peer_;
        uv_stream_t* stream_;

        void OnRead(uv_stream_t* stream, ssize_t nread, const uv_buf_t* buff);
    public:
        BlockChainServerShell(uv_loop_t* loop, PeerSession* peer);
        ~BlockChainServerShell(){}

        uv_stream_t* GetStream() const{
            return stream_;
        }

        PeerSession* GetPeer() const{
            return peer_;
        }
    };
}

#endif //TOKEN_SHELL_H