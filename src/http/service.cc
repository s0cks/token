#include "http/service.h"
#include "http/session.h"

namespace Token{
  void HttpService::AllocBuffer(uv_handle_t* handle, size_t suggested_size, uv_buf_t* buff){
    HttpSession* session = (HttpSession*) handle->data;
    session->InitReadBuffer(buff, 65536);
  }

  void HttpService::OnClose(uv_handle_t* handle){}

  void HttpService::OnWalk(uv_handle_t* handle, void* data){
    uv_close(handle, &OnClose);
  }

  bool HttpService::ShutdownService(uv_loop_t* loop){
    uv_stop(loop);

    int result;
    if((result = uv_loop_close(loop)) == UV_EBUSY){
      uv_walk(loop, &OnWalk, NULL);
      uv_run(loop, UV_RUN_DEFAULT);
      uv_stop(loop);
      if((result = uv_loop_close(loop)) != 0){
        LOG(INFO) << "shutdown result: " << uv_strerror(result);
        return false;
      }
      return true;
    }

    if(result != 0){
      LOG(INFO) << "shutdown result: " << uv_strerror(result);
      return false;
    }
    return true;
  }

  bool HttpService::Bind(uv_tcp_t* server, const int32_t& port){
    sockaddr_in bind_address;
    uv_ip4_addr("0.0.0.0", port, &bind_address);
    int err;
    if((err = uv_tcp_bind(server, (struct sockaddr*) &bind_address, 0)) != 0){
      LOG(WARNING) << "couldn't bind the health check service on port " << port << ": " << uv_strerror(err);
      return false;
    }
    return true;
  }

  bool HttpService::Accept(uv_stream_t* stream, HttpSession* session){
    int result;
    if((result = uv_accept(stream, session->GetStream())) != 0){
      LOG(WARNING) << "server accept failure: " << uv_strerror(result);
      return false;
    }
    return true;
  }

  void HttpService::SendNotSupported(HttpSession* session, const HttpRequestPtr& request){
    std::stringstream ss;
    ss << "Not Supported.";
    std::string body = ss.str();
    HttpResponsePtr resp = HttpTextResponse::NewInstance(session, STATUS_CODE_NOTSUPPORTED, body);
    session->Send(resp);
  }

  void HttpService::SendNotFound(HttpSession* session, const HttpRequestPtr& request){
    std::stringstream ss;
    ss << "Not Found: " << request->GetPath();
    std::string body = ss.str();
    HttpResponsePtr resp = HttpTextResponse::NewInstance(session, STATUS_CODE_NOTFOUND, body);
    session->Send(resp);
  }
}