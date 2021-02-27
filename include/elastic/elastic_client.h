#ifndef TOKEN_ELASTIC_CLIENT_H
#define TOKEN_ELASTIC_CLIENT_H

#ifdef TOKEN_ENABLE_ELASTICSEARCH

#include <uv.h>
#include "address.h"
#include "http/http_session.h"
#include "http/http_request.h"
#include "http/http_response.h"

namespace token{
  class ESClient : public HttpSession{
   private:
    NodeAddress address_;
    HttpRequestPtr request_;

    static void AllocBuffer(uv_handle_t* handle, size_t suggested_size, uv_buf_t* buff){
      buff->base = (char*)malloc(suggested_size);
      buff->len = suggested_size;
    }

    static void OnConnect(uv_connect_t* handle, int status){
      ESClient* client = (ESClient*)handle->data;
      if(status != 0){
        LOG(ERROR) << "connect failure: " << uv_strerror(status);
        client->CloseConnection();
        return;
      }

      client->Send(client->request_);

      int err;
      if((err = uv_read_start(client->GetStream(), &AllocBuffer, &OnMessageReceived)) != 0){
        LOG(ERROR) << "read failure: " << uv_strerror(err);
        client->CloseConnection();
        return;
      }
    }

    static void OnMessageReceived(uv_stream_t* stream, ssize_t nread, const uv_buf_t* buff){
      ESClient* client = (ESClient*)stream->data;
      if(nread == UV_EOF){
        LOG(ERROR) << "client disconnected!";
        return;
      } else if(nread < 0){
        LOG(ERROR) << "[" << nread << "] client read error: " << std::string(uv_strerror(nread));
        return;
      } else if(nread == 0){
        LOG(WARNING) << "zero message size received";
        return;
      } else if(nread > 65536){
        LOG(ERROR) << "too large of a buffer: " << nread;
        return;
      }

      BufferPtr buffer = Buffer::From(buff->base, nread);
      HttpResponsePtr response = HttpResponse::From(nullptr, buffer);
      LOG(INFO) << "response: " << std::endl << response->GetBody();
      client->CloseConnection();
      free(buff->base);
    }

    bool Connect(){
      int err;

      struct sockaddr_in address;
      if((err = uv_ip4_addr(address_.GetAddress().c_str(), address_.GetPort(), &address)) != 0){
        LOG(ERROR) << "uv_ip4_addr failure: " << uv_strerror(err);
        return false;
      }

#ifdef TOKEN_DEBUG
      LOG(INFO) << "creating connection to elasticsearch @ " << address_ << ".....";
#endif//TOKEN_DEBUG
      uv_connect_t request;
      request.data = this;
      if((err = uv_tcp_connect(&request, &handle_, (const struct sockaddr*)&address, &OnConnect)) != 0){
        LOG(ERROR) << "couldn't connect to elasticsearch @ " << address_ << ": " << uv_strerror(err);
        return false;
      }

      if((err = uv_run(loop_, UV_RUN_DEFAULT)) != 0){
        LOG(WARNING) << "uv_run failure: " << uv_strerror(err);
        return false;
      }
      return true;
    }

    void SetRequest(const HttpRequestPtr& request){
      request_ = request;
    }
   public:
    ESClient(const NodeAddress& addr):
      HttpSession(uv_loop_new()),
      address_(addr),
      request_(){}
    ~ESClient() = default;

    HttpRequestPtr GetRequest() const{
      return request_;
    }

    bool SendRequest(const HttpRequestPtr& request){
      SetRequest(request);
      return Connect();
    }
  };
}

#endif//TOKEN_ENABLE_ELASTICSEARCH
#endif//TOKEN_ELASTIC_CLIENT_H