#include "http/http_client.h"

namespace token{
  class HttpClientSession : public http::Session{
   private:
    utils::Address address_;
    uv_connect_t connect_;
    http::RequestPtr request_;
    http::ResponsePtr response_;

    static void AllocBuffer(uv_handle_t* handle, size_t suggested_size, uv_buf_t* buff){
      buff->base = (char*)malloc(suggested_size);
      buff->len = suggested_size;
    }

    static void OnConnect(uv_connect_t* handle, int status){
      auto session = (HttpClientSession*)handle->data;
      if(status != 0){
        LOG(ERROR) << "connect failure: " << uv_strerror(status);
        return session->CloseConnection();
      }

      session->Send(session->request_);

      int err;
      if((err = uv_read_start(session->GetStream(), &AllocBuffer, &OnMessageReceived)) != 0){
        LOG(ERROR) << "read failure: " << uv_strerror(err);
        return session->CloseConnection();
      }
    }

    static void OnMessageReceived(uv_stream_t* stream, ssize_t nread, const uv_buf_t* buff){
      std::shared_ptr<HttpClientSession> session = std::shared_ptr<HttpClientSession>((HttpClientSession*)stream->data);
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

      BufferPtr buffer = internal::CopyBufferFrom((uint8_t *) buff->base, nread);
      http::ResponsePtr response = http::ResponseParser::ParseResponse(buffer);
      session->SetResponse(response);
      free(buff->base);
      return session->CloseConnection();
    }

    void SetRequest(const http::RequestPtr& request){
      request_ = request;
    }

    void SetResponse(const http::ResponsePtr& response){
      response_ = response;
    }
   public:
    HttpClientSession(const utils::Address& address, const http::RequestPtr& request):
      http::Session(uv_loop_new()),
      address_(address),
      connect_(),
      request_(request),
      response_(nullptr){
      connect_.data = this;
    }
    ~HttpClientSession() override{
      uv_loop_close(loop_);
      free(loop_);
    }

    http::RequestPtr GetRequest() const{
      return request_;
    }

    http::ResponsePtr GetResponse() const{
      return response_;
    }

    bool Connect(){
      int err;

      struct sockaddr_in address;
      if((err = uv_ip4_addr(address_.ToString().data(), address_.port(), &address)) != 0){
        LOG(ERROR) << "uv_ip4_addr failure: " << uv_strerror(err);
        return false;
      }

      DLOG(INFO) << "creating connection to " << address_ << "....";
      if((err = uv_tcp_connect(&connect_, &handle_, (const struct sockaddr*)&address, &OnConnect)) != 0){
        LOG(ERROR) << "couldn't connect to elasticsearch @ " << address_ << ": " << uv_strerror(err);
        return false;
      }

      if((err = uv_run(loop_, UV_RUN_DEFAULT)) != 0){
        LOG(WARNING) << "uv_run failure: " << uv_strerror(err);
        return false;
      }
      return true;
    }
  };

  http::ResponsePtr HttpClient::Send(const http::RequestPtr& request){
    HttpClientSession session(address_, request);
    if(!session.Connect()){
#ifdef TOKEN_DEBUG
      LOG(WARNING) << "cannot send " << request->ToString() << " to: " << address_;
#endif//TOKEN_DEBUG
      return nullptr;
    }
    return session.GetResponse();
  }
}