#ifndef TOKEN_ELASTIC_CLIENT_H
#define TOKEN_ELASTIC_CLIENT_H

#ifdef TOKEN_ENABLE_ELASTICSEARCH

#include <uv.h>
#include "address.h"

namespace token{
  class ESClient{
   private:
    NodeAddress address_;
    uv_loop_t* loop_;
    uv_connect_t request_;
    uv_tcp_t handle_;
   private:
    void Send(const std::string& message);
    void CloseConnection();
    static void OnClose(uv_handle_t* handle);
    static void OnWalk(uv_handle_t* handle, void* arg);
    static void AllocBuffer(uv_handle_t* handle, size_t suggested_size, uv_buf_t* buff);
    static void OnConnect(uv_connect_t* conn, int status);
    static void OnMessageSent(uv_write_t* request, int status);
    static void OnMessageReceived(uv_stream_t* stream, ssize_t nread, const uv_buf_t* buff);
   public:
    ESClient(const NodeAddress& address):
      address_(address),
      loop_(uv_loop_new()),
      request_(),
      handle_(){
      request_.data = this;
      handle_.data = this;

      int err;
      if((err = uv_tcp_init(loop_, &handle_)) != 0){
        LOG(ERROR) << "couldn't initialize the elasticsearch client handle: " << uv_strerror(err);
        return;
      }
    }
    ~ESClient(){}

    uv_stream_t*
    GetStream() const{
      return (uv_stream_t*)&handle_;
    }

    bool Connect();
  };
}

#endif//TOKEN_ENABLE_ELASTICSEARCH
#endif//TOKEN_ELASTIC_CLIENT_H