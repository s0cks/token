#ifndef TOKEN_SESSION_H
#define TOKEN_SESSION_H

#include "message.h"
#include "utils/buffer.h"
#include "atomic/relaxed_atomic.h"

namespace token{
#define FOR_EACH_SESSION_STATE(V) \
    V(Connecting)                 \
    V(Connected)                  \
    V(Disconnected)

  template<class M>
  class Server;

  template<class M>
  class Session{
    friend class Server<M>;
   private:
    typedef Session<M> SessionType;
    typedef Server<M> ServerType;
    typedef std::shared_ptr<M> SessionMessagePtr;
    typedef std::vector<SessionMessagePtr> SessionMessageList;

    struct SessionWriteData{
      uv_write_t request;
      SessionType* session;
      BufferPtr buffer;

      SessionWriteData(SessionType* s, int64_t size):
        request(),
        session(s),
        buffer(Buffer::NewInstance(size)){}
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
    uv_loop_t* loop_;
    uv_tcp_t handle_;
    RelaxedAtomic<State> state_;

    void SetState(const State& state){
      state_ = state;
    }

    virtual void OnMessageRead(const SessionMessagePtr& message) = 0;

    void SendMessages(const SessionMessageList& messages){
      size_t total_messages = messages.size();
      if(total_messages <= 0){
        LOG(WARNING) << "not sending any messages!";
        return;
      }

      int64_t total_size = 0;
      std::for_each(messages.begin(), messages.end(), [&total_size](const SessionMessagePtr& msg){
        total_size += msg->GetBufferSize();
      });

      LOG(INFO) << "sending " << total_messages << " messages....";
      SessionWriteData* data = new SessionWriteData(this, total_size);
      uv_buf_t buffers[total_messages];

      int64_t offset = 0;
      for(size_t idx = 0; idx < total_messages; idx++){
        const SessionMessagePtr& msg = messages[idx];
#ifdef TOKEN_DEBUG
        LOG(INFO) << "sending " << msg->ToString();
#endif//TOKEN_DEBUG
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

    static void
    OnMessageSent(uv_write_t* req, int status){
      if(status != 0)
        LOG(ERROR) << "failed to send message: " << uv_strerror(status);
      delete ((SessionWriteData*)req->data);
    }
   public:
    Session(uv_loop_t* loop):
      loop_(loop),
      handle_(),
      state_(Session::kDisconnectedState){
      handle_.data = this;

      int err;
      if((err = uv_tcp_init(loop_, &handle_)) != 0){
        LOG(ERROR) << "couldn't initialize the channel handle: " << uv_strerror(err);
        return;
      }

      if((err = uv_tcp_keepalive(GetHandle(), 1, 60)) != 0){
        LOG(WARNING) << "couldn't configure channel keep-alive: " << uv_strerror(err);
        return;
      }
    }
    virtual ~Session() = default;

    State GetState() const{
      return state_;
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

#define DEFINE_STATE_CHECK(Name) \
    inline bool Is##Name(){ return GetState() == Session::k##Name##State; }
    FOR_EACH_SESSION_STATE(DEFINE_STATE_CHECK)
#undef DEFINE_STATE_CHECK
  };
}

#endif//TOKEN_SESSION_H