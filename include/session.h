#ifndef TOKEN_SESSION_H
#define TOKEN_SESSION_H

#include "message.h"
#include "vthread.h"
#include "buffer.h"
#include "atomic/relaxed_atomic.h"

namespace token{
#define LOG_SESSION(LevelName, Session) \
  LOG(LevelName) << "[session-" << (Session)->GetUUID().ToStringAbbreviated() << "] "

#define DLOG_SESSION(LevelName, Session) \
  DLOG(LevelName) << "[session-" << (Session)->GetUUID().ToStringAbbreviated() << "] "

#define DLOG_SESSION_IF(LevelName, Condition, Session) \
  DLOG_IF(LevelName, Condition) << "[session-" << (Session)->GetUUID().ToStringAbbreviated() << "] "

#define FOR_EACH_SESSION_STATE(V) \
    V(Connecting)                 \
    V(Connected)                  \
    V(Disconnected)

  template<class M>
  class Server;

  template<class M>
  class Session{
    friend class Server<M>;
   protected:
    typedef M SessionMessageType;
    typedef std::shared_ptr<SessionMessageType> SessionMessageTypePtr;
    typedef std::vector<SessionMessageTypePtr> SessionMessageTypeList;

    typedef Session<M> BaseType;
    typedef Server<M> ServerType;

    friend SessionMessageTypeList& operator<<(SessionMessageTypeList& messages, const SessionMessageTypePtr& msg){
      messages.push_back(msg);
      return messages;
    }
   private:
    struct SessionWriteData{
      uv_write_t request;
      BaseType* session;
      BufferPtr buffer;

      SessionWriteData(BaseType* s, int64_t size):
        request(),
        session(s),
        buffer(Buffer::NewInstance(size)){
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
    RelaxedAtomic<State> state_;

    void SetState(const State& state){
      state_ = state;
    }

    void SetUUID(const UUID& uuid){
      uuid_ = uuid;
    }

    virtual void OnMessageRead(const SessionMessageTypePtr& message) = 0;

    static void OnClose(uv_handle_t* handle){
      DLOG(INFO) << "on-close.";
    }

    static void OnWalk(uv_handle_t* handle, void* arg){
      uv_close(handle, &OnClose);
    }

    void CloseConnection(){
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
    Session(uv_loop_t* loop, const UUID& uuid, const int32_t& keep_alive=0):
      uuid_(uuid),
      loop_(loop),
      handle_(),
      state_(Session::kDisconnectedState){
      handle_.data = this;
      if(loop){
        CHECK_UVRESULT(uv_tcp_init(loop_, &handle_), LOG_SESSION(ERROR, this), "couldn't initialize the session handle");
        if(keep_alive > 0)
          CHECK_UVRESULT(uv_tcp_keepalive(&handle_, true, keep_alive), LOG_SESSION(ERROR, this), "couldn't enable session keep-alive");
      }
    }
    Session():
      uuid_(),
      loop_(nullptr),
      handle_(),
      state_(Session::kDisconnectedState){
      handle_.data = this;
    }
    virtual ~Session() = default;

    UUID GetUUID() const{
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

    virtual void SendMessages(const SessionMessageTypeList& messages){
      size_t total_messages = messages.size();
      if(total_messages <= 0){
        LOG_SESSION(WARNING, this) << "not sending any messages!";
        return;
      }

      int64_t total_size = 0;
      std::for_each(messages.begin(), messages.end(), [&total_size](const SessionMessageTypePtr& msg){
        total_size += msg->GetBufferSize();
      });

      DLOG_SESSION(INFO, this) << "sending " << total_messages << " messages....";

      auto data = new SessionWriteData(this, total_size);
      uv_buf_t buffers[total_messages];

      int64_t offset = 0;
      for(size_t idx = 0; idx < total_messages; idx++){
        const SessionMessageTypePtr& msg = messages[idx];
        int64_t msize = msg->GetBufferSize();
        if(!msg->Write(data->buffer)){
          LOG_SESSION(ERROR, this) << "couldn't serialize message #" << idx << " " << msg->ToString() << " (" << msize << ")";
          return;
        }

        DLOG_SESSION(INFO, this) << "sending message #" << idx << " " << msg->ToString() << " (" << msize << "b)";

        int64_t msg_size = msg->GetBufferSize();
        buffers[idx].base = &data->buffer->data()[offset];
        buffers[idx].len = msg_size;
        offset += msg_size;
      }
      uv_write(&data->request, GetStream(), buffers, total_messages, &OnMessageSent);
    }

#define DEFINE_STATE_CHECK(Name) \
    inline bool Is##Name(){ return GetState() == Session::k##Name##State; }
    FOR_EACH_SESSION_STATE(DEFINE_STATE_CHECK)
#undef DEFINE_STATE_CHECK
  };
}

#endif//TOKEN_SESSION_H