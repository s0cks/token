#ifndef TOKEN_SESSION_H
#define TOKEN_SESSION_H

#include "uuid.h"
#include "os_thread.h"

#include "server/message.h"
#include "atomic/relaxed_atomic.h"

namespace token{
#define LOG_SESSION(LevelName, Session) \
  LOG(LevelName) << "[session-" << (Session)->GetUUID().ToString() << "] "

#define DLOG_SESSION(LevelName, Session) \
  DLOG(LevelName) << "[session-" << (Session)->GetUUID().ToString() << "] "

#define DLOG_SESSION_IF(LevelName, Condition, Session) \
  DLOG_IF(LevelName, Condition) << "[session-" << (Session)->GetUUID().ToString() << "] "

#define DVLOG_SESSION(Level, Session) \
  DVLOG(Level) << "[session-" << (Session)->GetUUID().ToString() << "] "

#define FOR_EACH_SESSION_STATE(V) \
    V(Connecting)                 \
    V(Connected)                  \
    V(Disconnected)

  class SessionBase{
   protected:
    struct SessionWriteData{
      uv_write_t request;
      SessionBase* session;
      internal::BufferPtr buffer;

      SessionWriteData(SessionBase* s, uint64_t size):
        request(),
        session(s),
        buffer(internal::NewBuffer(size)){
        request.data = this;
      }
    };
   public:
    enum State{
#define DEFINE_STATE(Name) k##Name##State,
      FOR_EACH_SESSION_STATE(DEFINE_STATE)
#undef DEFINE_STATE
    };

    friend std::ostream& operator<<(std::ostream& stream, const State& state){
      switch(state){
#define DEFINE_TOSTRING(Name) \
        case State::k##Name##State: \
          return stream << #Name;
        FOR_EACH_SESSION_STATE(DEFINE_TOSTRING)
#undef DEFINE_TOSTRING
        default:
          return stream << "Unknown";
      }
    }
   protected:
    UUID uuid_;
    uv_loop_t* loop_;
    uv_tcp_t handle_;
    atomic::RelaxedAtomic<State> state_;

    void SetState(const State& state){
      state_ = state;
    }

    void SetUUID(const UUID& uuid){
      uuid_ = uuid;
    }

    static void OnClose(uv_handle_t* handle){
      DLOG(INFO) << "on-close.";
    }

    static void OnWalk(uv_handle_t* handle, void* arg){
      uv_close(handle, &OnClose);
    }

    void CloseConnection(){
      DLOG(INFO) << "closing session.";
      uv_walk(loop_, &OnWalk, nullptr);
    }

    static void
    OnMessageSent(uv_write_t* req, int status){
      auto data = (SessionWriteData*)req->data;
      if(status != 0)
        LOG_SESSION(WARNING, data->session) << "failed to send message: " << uv_strerror(status); //TODO: use SESSION_LOG
      delete data;
    }
   public:
    SessionBase(uv_loop_t* loop, const UUID& uuid, const int32_t& keep_alive=0):
      uuid_(uuid),
      loop_(loop),
      handle_(),
      state_(SessionBase::kDisconnectedState){
      handle_.data = this;
      if(loop){
        CHECK_UVRESULT(uv_tcp_init(loop_, &handle_), LOG_SESSION(ERROR, this), "couldn't initialize the session handle");
        if(keep_alive > 0)
          CHECK_UVRESULT(uv_tcp_keepalive(&handle_, true, keep_alive), LOG_SESSION(ERROR, this), "couldn't enable session keep-alive");
      }
    }
    SessionBase():
      uuid_(),
      loop_(nullptr),
      handle_(),
      state_(SessionBase::kDisconnectedState){
      handle_.data = this;
    }
    virtual ~SessionBase() = default;

    UUID GetUUID() const{//TODO: rename
      return uuid_;
    }

    State GetState() const{
      return (State)state_;
    }

    uv_loop_t* GetLoop() const{
      return loop_;
    }

    uv_tcp_t* GetHandle(){
      return &handle_;
    }

    uv_stream_t* GetStream(){
      return (uv_stream_t*)&handle_;
    }

    virtual void SendMessages(const std::vector<std::shared_ptr<MessageBase>>& messages){
      size_t total_messages = messages.size();
      if(total_messages <= 0){
        LOG_SESSION(WARNING, this) << "not sending any messages!";
        return;
      }

      uint64_t total_size = 0;
      std::for_each(messages.begin(), messages.end(), [&total_size](const std::shared_ptr<MessageBase>& msg){
        total_size += msg->GetBufferSize();
      });

      DVLOG_SESSION(2, this) << "sending " << total_messages << " messages....";
      auto data = new SessionWriteData(this, total_size);
      uv_buf_t buffers[total_messages];

      uint64_t offset = 0;
      for(size_t idx = 0; idx < total_messages; idx++){
        const std::shared_ptr<MessageBase>& msg = messages[idx];
        uint64_t msize = msg->GetBufferSize();
        DVLOG_SESSION(1, this) << "sending message #" << idx << " " << msg->ToString() << " (" << msize << "b)";
        auto msg_buff = msg->ToBuffer();
        data->buffer->PutBytes(msg_buff);

        uint64_t msg_size = msg->GetBufferSize();
        buffers[idx].base = (char*)&data->buffer->data()[offset];
        buffers[idx].len = msg_size;
        offset += msg_size;
      }
      uv_write(&data->request, GetStream(), buffers, total_messages, &OnMessageSent);
    }

#define DEFINE_STATE_CHECK(Name) \
    inline bool Is##Name(){ return GetState() == SessionBase::k##Name##State; }
    FOR_EACH_SESSION_STATE(DEFINE_STATE_CHECK)
#undef DEFINE_STATE_CHECK
  };
}

#endif//TOKEN_SESSION_H