#ifndef TOKEN_UTILS_TCP_H
#define TOKEN_UTILS_TCP_H

#include <uv.h>

namespace Token{
  static inline bool
  ServerBind(uv_tcp_t* server, const int32_t port){
    sockaddr_in bind_address;
    uv_ip4_addr("0.0.0.0", port, &bind_address);
    int err;
    if((err = uv_tcp_bind(server, (struct sockaddr*)&bind_address, 0)) != 0){
      LOG(ERROR) << "server bind failure: " << uv_strerror(err);
      return false;
    }
    return true;
  }

  static inline bool
  ServerListen(uv_stream_t* server, uv_connection_cb on_connect, const int backlog = 100){
    int err;
    if((err = uv_listen(server, backlog, on_connect)) != 0){
      LOG(ERROR) << "server accept failure: " << uv_strerror(err);
      return false;
    }
    return true;
  }

  static inline bool
  ServerAccept(uv_stream_t* server, uv_stream_t* client){
    int err;
    if((err = uv_accept(server, client)) != 0){
      LOG(ERROR) << "server accept failure: " << uv_strerror(err);
      return false;
    }
    return true;
  }

  static inline bool
  ServerReadStart(uv_stream_t* stream, uv_alloc_cb on_alloc, uv_read_cb on_read){
    int err;
    if((err = uv_read_start(stream, on_alloc, on_read)) != 0){
      LOG(ERROR) << "server read failure: " << uv_strerror(err);
      return false;
    }
    return true;
  }
}

#endif //TOKEN_UTILS_TCP_H