#include "http/session.h"
#include "http/health/health_service.h"

namespace Token{
  struct HttpSessionWriteRequestData{
    uv_write_t request;
    HttpSession* session;
    BufferPtr buffer;

    HttpSessionWriteRequestData(HttpSession* s, const HttpResponsePtr& response):
      request(),
      session(s),
      buffer(Buffer::NewInstance(response->GetBufferSize())){
      request.data = this;
    }
  };

  void HttpSession::Send(const std::shared_ptr<HttpResponse>& response){
    HttpSessionWriteRequestData* data = new HttpSessionWriteRequestData(this, response);
    if(!response->Write(data->buffer)){
      LOG(WARNING) << "couldn't encode http response: " << response->ToString();
      return;
    }

    uv_buf_t buff;
    buff.base = data->buffer->data();
    buff.len = data->buffer->GetWrittenBytes();
    uv_write(&data->request, GetStream(), &buff, 1, &OnResponseSent);
  }

  void HttpSession::OnResponseSent(uv_write_t* req, int status){
    if(status != 0)
      LOG(WARNING) << "failed to send the response: " << uv_strerror(status);
    if(req->data){
      delete ((HttpSessionWriteRequestData*) req->data);
    }
  }

  void HttpSession::Close(){
    if(uv_is_closing((uv_handle_t*) &handle_)){
      return;
    }
    uv_close((uv_handle_t*) &handle_, OnClose);
  }

  void HttpSession::OnClose(uv_handle_t* handle){
    //TODO: do something
  }
}