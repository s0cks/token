#include "json.h"
#include "http/http_response.h"
#include "elastic/elastic_client.h"

namespace token{
  void ESClient::OnConnect(uv_connect_t* conn, int status){
    ESClient* client = (ESClient*)conn->data;
    if(status != 0){
      LOG(ERROR) << "connect failure: " << uv_strerror(status);
      client->CloseConnection();
      return;
    }

    client->Send("Hello World");

    int err;
    if((err = uv_read_start(client->GetStream(), &AllocBuffer, &OnMessageReceived)) != 0){
      LOG(ERROR) << "read failure: " << uv_strerror(err);
      client->CloseConnection();
      return;
    }
  }

  void ESClient::OnMessageReceived(uv_stream_t* stream, ssize_t nread, const uv_buf_t* buff){
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

  void ESClient::OnClose(uv_handle_t* handle){
#ifdef TOKEN_DEBUG
    LOG(INFO) << "closing handle....";
#endif//TOKEN_DEBUG
  }

  void ESClient::AllocBuffer(uv_handle_t* handle, size_t suggested_size, uv_buf_t* buff){
    buff->base = (char*)malloc(suggested_size);
    buff->len = suggested_size;
  }

  void ESClient::OnWalk(uv_handle_t* handle, void* arg){
    uv_close(handle, &OnClose);
  }

  void ESClient::CloseConnection(){
    uv_walk(loop_, &OnWalk, NULL);
  }

  struct ClientWriteData{
    uv_write_t request;
    ESClient* session;
    BufferPtr buffer;

    ClientWriteData(ESClient* s, const BufferPtr& b):
      request(),
      session(s),
      buffer(b){
      request.data = this;
    }
  };

  void ESClient::OnMessageSent(uv_write_t* request, int status){
    ClientWriteData* data = (ClientWriteData*)request->data;
    LOG(INFO) << "message sent!";
    if(status != 0)
      LOG(WARNING) << "send failure: " << uv_strerror(status);
    delete data;
  }

  void ESClient::Send(const std::string& message){
    Json::String body;
    Json::Writer writer(body);
    writer.StartObject();
      Json::SetField(writer, "@timestamp", FormatTimestampElastic(Clock::now()));
      Json::SetField(writer, "message", message);
    writer.EndObject();

    HttpHeadersMap headers;
    SetHttpHeader(headers, "Content-Type", HTTP_CONTENT_TYPE_APPLICATION_JSON);
    SetHttpHeader(headers, "Content-Length", body.GetSize());

    BufferPtr buffer = Buffer::NewInstance(65535);

    buffer->PutString("POST /my-index-000001/_doc?pretty HTTP/1.1\r\n");
    for(auto& it : headers){
      std::stringstream ss;
      ss << it.first << ": " << it.second << "\r\n";
      buffer->PutString(ss.str());
    }

    buffer->PutString("\r\n");
    buffer->PutBytes((uint8_t*)body.GetString(), body.GetSize());

#ifdef TOKEN_DEBUG
    LOG(INFO) << "sending: " << std::endl << std::string(buffer->data(), buffer->GetWrittenBytes());
#endif//TOKEN_DEBUG

    ClientWriteData* data = new ClientWriteData(this, buffer);

    uv_buf_t buff;
    buff.base = buffer->data();
    buff.len = buffer->GetWrittenBytes();

    uv_write(&data->request, GetStream(), &buff, 1, &OnMessageSent);
  }

  bool ESClient::Connect(){
    int err;

    struct sockaddr_in address;
    if((err = uv_ip4_addr(address_.GetAddress().c_str(), address_.GetPort(), &address)) != 0){
      LOG(ERROR) << "uv_ip4_addr failure: " << uv_strerror(err);
      return false;
    }

#ifdef TOKEN_DEBUG
    LOG(INFO) << "creating connection to elasticsearch " << address_ << "....";
#endif//TOKEN_DEBUG
    if((err = uv_tcp_connect(&request_, &handle_, (const struct sockaddr*)&address, &OnConnect)) != 0){
      LOG(ERROR) << "couldn't connect to elasticsearch " << address_ << ": " << uv_strerror(err);
      return false;
    }

#ifdef TOKEN_DEBUG
    LOG(INFO) << "starting elasticsearch client loop.....";
#endif//TOKEN_DEBUG
    if((err = uv_run(loop_, UV_RUN_DEFAULT)) != 0){
      LOG(ERROR) << "couldn't run elasticsearch client loop: " << uv_strerror(err);
      return false;
    }
    return true;
  }
}