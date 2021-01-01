#include "http/session.h"
#include "http/healthcheck_service.h"

namespace Token{
  struct HttpSessionWriteRequestData{
    HttpSession *session;
    BufferPtr buffer;

    HttpSessionWriteRequestData(HttpSession *s, int64_t size):
        session(s),
        buffer(Buffer::NewInstance(size)){}
  };

  void HttpSession::Send(HttpResponse *response){
    int64_t content_length = response->GetBufferSize();

    uv_write_t *req = (uv_write_t *) malloc(sizeof(uv_write_t));
    req->data = new HttpSessionWriteRequestData(this, content_length);
    BufferPtr &buffer = ((HttpSessionWriteRequestData *) req->data)->buffer;
    response->Write(buffer);

    uv_buf_t buff;
    buff.base = buffer->data();
    buff.len = buffer->GetWrittenBytes();
    uv_write(req, (uv_stream_t *) &handle_, &buff, 1, &OnResponseSent);
  }

  void HttpSession::OnResponseSent(uv_write_t *req, int status){
    if(status != 0)
      LOG(WARNING) << "failed to send the response: " << uv_strerror(status);
    free(req->data);
    free(req);
  }

  void HttpSession::Close(){
    if(uv_is_closing((uv_handle_t *) &handle_))
      return;
    uv_close((uv_handle_t *) &handle_, OnClose);
  }

  void HttpSession::OnClose(uv_handle_t *handle){
    //TODO: do something
  }
}