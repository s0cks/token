#include "node/peer.h"

namespace Token{
    void* PeerSession::PeerSessionThread(void* data){
        PeerSession* session = (PeerSession*)data;
        LOG(INFO) << "connecting to: "; //TODO: log address
        uv_loop_t* loop = uv_loop_new();
        uv_tcp_t socket;
        uv_tcp_init(loop, &socket);
        uv_tcp_keepalive(&socket, 1, 60);

        struct sockaddr_in addr;
        uv_ip4_addr(session->GetAddress(), session->GetPort(), &addr);
        int err;
        if((err = uv_tcp_connect(&session->conn_, &socket, (const struct sockaddr*)&addr, &OnConnect)) != 0){
            LOG(ERROR) << "cannot connect to peer: " << uv_strerror(err);
            return nullptr; //TODO: fix error handling
        }

        uv_run(loop, UV_RUN_DEFAULT);
        return nullptr;
    }

    void PeerSession::OnConnect(uv_connect_t* conn, int status){
        PeerSession* session = (PeerSession*)conn->data;
    }
}