#ifdef TOKEN_ENABLE_SERVER

#include "session.h"

namespace Token{
#define LOCK std::unique_lock<std::mutex> lock(mutex_)
#define WAIT cond_.wait(lock)
#define SIGNAL_ONE cond_.notify_one()
#define SIGNAL_ALL cond_.notify_all()
#define LOCK_GUARD std::lock_guard<std::mutex> guard(mutex_)

#define SESSION_OK_STATUS(Status, Message) \
    LOG(INFO) << (Message);             \
    SetStatus((Status));
#define SESSION_OK(Message) \
    SESSION_OK_STATUS(Session::kOk, (Message));
#define SESSION_WARNING_STATUS(Status, Message) \
    LOG(WARNING) << (Message);               \
    SetStatus((Status));
#define SESSION_WARNING(Message) \
    SESSION_WARNING_STATUS(Session::kWarning, (Message))
#define SESSION_ERROR_STATUS(Status, Message) \
    LOG(ERROR) << (Message);               \
    SetStatus((Status));
#define SESSION_ERROR(Message) \
    SESSION_ERROR_STATUS(Session::kError, (Message))

  Session::Session(uv_loop_t* loop):
    Object(),
    mutex_(),
    cond_(),
    state_(State::kDisconnected),
    status_(Status::kOk),
    loop_(loop),
    handle_(){
    handle_.data = this;

    uv_tcp_keepalive(GetHandle(), 1, 60);
    int err;
    if((err = uv_tcp_init(loop, &handle_)) != 0){
      std::stringstream ss;
      ss << "couldn't initialize the session handle: " << uv_strerror(err);
      SESSION_ERROR(ss.str());
      return;
    }
  }

  void Session::AllocBuffer(uv_handle_t* handle, size_t suggested_size, uv_buf_t* buff){
    buff->len = suggested_size;
    buff->base = (char*)malloc(sizeof(uint8_t)*suggested_size);
  }

  void Session::SetState(Session::State state){
    state_ = state;
    SIGNAL_ALL;
  }

  void Session::SetStatus(Session::Status status){
    status_ = status;
    SIGNAL_ALL;
  }

  bool Session::WaitForState(State state, intptr_t timeout){
    LOCK;
    while(state_ != state) WAIT;
    return true;
  }

  bool Session::WaitForStatus(Status status, intptr_t timeout){
    LOCK;
    while(status_ != status) WAIT;
    return true;
  }

  Session::State Session::GetState(){
    LOCK_GUARD;
    return state_;
  }

  Session::Status Session::GetStatus(){
    LOCK_GUARD;
    return status_;
  }

  struct SessionWriteData{
    uv_write_t request;
    Session* session;
    BufferPtr buffer;

    SessionWriteData(Session* s):
      request(),
      session(s),
      buffer(Buffer::NewInstance(65536)){
      request.data = this;
    }
    ~SessionWriteData() = default;
  };

  void Session::Send(const MessagePtr& msg){
    MessageList messages = { msg };
    return Send(messages);
  }

  void Session::Send(std::vector<MessagePtr>& messages){
    size_t total_messages = messages.size();
    if(total_messages <= 0){
      LOG(WARNING) << "not sending any messages!";
      return;
    }

    LOG(INFO) << "sending " << total_messages << " messages....";
    SessionWriteData* data = new SessionWriteData(this);
    uv_buf_t buffers[total_messages];

    int64_t offset = 0;
    for(size_t idx = 0; idx < total_messages; idx++){
      MessagePtr& msg = messages[idx];
      LOG(INFO) << "sending " << msg->ToString();
      if(!msg->Write(data->buffer)){
        LOG(WARNING) << "couldn't write " << msg->ToString();
        return;
      }

      int64_t msg_size = msg->GetBufferSize();
      buffers[idx].base = &data->buffer->data()[offset];
      buffers[idx].len = msg_size;
      offset += msg_size;
    }

    uv_write(&data->request, GetStream(), buffers, total_messages, &OnMessageSent);
  }

  void Session::OnMessageSent(uv_write_t* req, int status){
    if(status != 0)
      LOG(ERROR) << "failed to send message: " << uv_strerror(status);
    delete (SessionWriteData*)req->data;
  }
}

#endif//TOKEN_ENABLE_SERVER