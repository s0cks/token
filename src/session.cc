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
    handle_(),
    rbuff_(Buffer::NewInstance(kBufferSize)),
    wbuff_(Buffer::NewInstance(kBufferSize)){
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
    Session* session = (Session*) handle->data;
    BufferPtr& rbuff = session->GetReadBuffer();
    buff->len = suggested_size;
    buff->base = rbuff->data();
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

  void Session::Send(const MessagePtr& msg){
#ifdef TOKEN_DEBUG
    LOG(INFO) << "sending " << msg->ToString() << " (" << msg->GetBufferSize() << " bytes)";
#endif//TOKEN_DEBUG

    BufferPtr& wbuff = GetWriteBuffer();
    if(!msg->Write(wbuff)){
      LOG(WARNING) << "couldn't serialize message " << msg->ToString();
      return;
    }

    uv_buf_t buff;
    buff.len = wbuff->GetWrittenBytes();
    buff.base = wbuff->data();

    uv_write_t* req = (uv_write_t*) malloc(sizeof(uv_write_t));
    req->data = this;
    uv_write(req, GetStream(), &buff, 1, &OnMessageSent);
  }

  void Session::Send(std::vector<MessagePtr>& messages){
    size_t total_messages = messages.size();
    if(total_messages <= 0){
      LOG(WARNING) << "not sending any messages!";
      return;
    }

  #ifdef TOKEN_DEBUG
    LOG(INFO) << "sending " << total_messages << " messages....";
  #endif//TOKEN_DEBUG

    BufferPtr& wbuff = GetWriteBuffer();
    uv_buf_t buffers[total_messages];

    int64_t offset = 0;
    for(size_t idx = 0; idx < total_messages; idx++){
      MessagePtr msg = messages[idx];
      int64_t total_size = msg->GetBufferSize();

#ifdef TOKEN_DEBUG
      LOG(INFO) << "sending " << msg->ToString() << " (" << total_size << " Bytes)";
#endif//TOKEN_DEBUG

      if(!msg->Write(wbuff)){
        LOG(WARNING) << "couldn't encode message: " << msg->ToString();
        return;
      }

      buffers[idx].len = total_size;
      buffers[idx].base = &wbuff->data()[offset];
      offset += total_size;
    }
    messages.clear();

    uv_write_t* req = (uv_write_t*) malloc(sizeof(uv_write_t));
    req->data = this;
    uv_write(req, GetStream(), buffers, total_messages, &OnMessageSent);
  }

  void Session::OnMessageSent(uv_write_t* req, int status){
    Session* session = (Session*) req->data;
    session->GetWriteBuffer()->Reset();
    if(status != 0)
      LOG(ERROR) << "failed to send message: " << uv_strerror(status);
    free(req);
  }

  /*void Session::SendInventory(std::vector<InventoryItem>& items){
      std::vector<Message*> data;

      size_t n = InventoryMessage::kMaxAmountOfItemsPerMessage;
      size_t size = (items.size() - 1) / n + 1;
      for(size_t idx = 0; idx < size; idx++){
          auto start = std::next(items.cbegin(), idx*n);
          auto end = std::next(items.cbegin(), idx*n+n);

          std::vector<InventoryItem> inv(n);
          if(idx*n+n > items.size()){
              end = items.cend();
              inv.resize(items.size()-idx*n);
          }
          std::copy(start, end, inv.begin());

          data.push_back(InventoryMessage::NewInstance(inv));
      }
      Send(data);
  }*/

  bool ThreadedSession::Disconnect(){
    if(pthread_self() == thread_){
      // Inside Session OSThreadBase
      uv_read_stop(GetStream());
      uv_stop(GetLoop());
      uv_loop_close(GetLoop());
      SetState(State::kDisconnected);
    } else{
      // Outside Session OSThreadBase
      uv_async_send(&shutdown_);
    }
    return true;
  }
}

#endif//TOKEN_ENABLE_SERVER