#ifndef TOKEN_SESSION_H
#define TOKEN_SESSION_H

#include "message.h"
#include "server/channel.h"
#include "atomic/relaxed_atomic.h"

namespace Token{
  template<class S>
  class SessionWriteData{
   protected:
    S* session_;
    uv_write_t request_;
    BufferPtr buffer_;
   public:
    SessionWriteData(S* session, const int64_t& size):
      session_(session),
      request_(),
      buffer_(Buffer::NewInstance(size)){
      request_.data = this;
    }
    virtual ~SessionWriteData() = default;

    S* GetSession() const{
      return session_;
    }

    BufferPtr GetBuffer() const{
      return buffer_;
    }

    uv_write_t* GetRequest(){
      return &request_;
    }
  };

#define FOR_EACH_SESSION_STATE(V) \
    V(Connecting)                 \
    V(Connected)                  \
    V(Disconnected)

  template<class M, class S>
  class Server;

  template<class M>
  class Session : public Channel<M>{
    using Base = Session<M>;
    friend class Server<M, Base>;
   private:
    using SessionMessagePtr = std::shared_ptr<M>;
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
    RelaxedAtomic<State> state_;

    void SetState(const State& state){
      state_ = state;
    }

    virtual void OnMessageRead(const SessionMessagePtr& message) = 0;

    static void
    AllocBuffer(uv_handle_t* handle, size_t suggested_size, uv_buf_t* buf){
      buf->base = (char*)malloc(suggested_size);
      buf->len = suggested_size;
    }
   public:
    Session(uv_loop_t* loop):
      Channel<M>(loop),
      state_(Session::kDisconnectedState){}
    virtual ~Session() = default;

    State GetState() const{
      return state_;
    }

#define DEFINE_STATE_CHECK(Name) \
    inline bool Is##Name(){ return GetState() == Session::k##Name##State; }
    FOR_EACH_SESSION_STATE(DEFINE_STATE_CHECK)
#undef DEFINE_STATE_CHECK
  };

  template<class M, class S>
  class Server;

  class RpcSession : public Session<RpcMessage>{
   protected:
    RpcSession(uv_loop_t* loop):
      Session<RpcMessage>(loop){}

#define DECLARE_MESSAGE_HANDLER(Name) \
    virtual void On##Name##Message(const Name##MessagePtr& message) = 0;
    FOR_EACH_MESSAGE_TYPE(DECLARE_MESSAGE_HANDLER)
#undef DECLARE_MESSAGE_HANDLER

    void OnMessageRead(const RpcMessagePtr& message){
      switch(message->GetMessageType()){
#define DEFINE_HANDLE(Name) \
        case RpcMessage::MessageType::k##Name##MessageType: \
          On##Name##Message(std::static_pointer_cast<Name##Message>(message)); \
          break;
        FOR_EACH_MESSAGE_TYPE(DEFINE_HANDLE)
#undef DEFINE_HANDLE
        default:
          break;
      }
    }
   public:
    virtual ~RpcSession() = default;

    inline void Send(const RpcMessageList& messages){
      return SendMessages(messages);
    }

    inline void Send(const RpcMessagePtr& msg){
      RpcMessageList messages = { msg };
      return SendMessages(messages);
    }

#define DEFINE_SEND(Name) \
    inline void           \
    Send(const Name##MessagePtr& msg){ \
      RpcMessageList messages = { msg };  \
      return SendMessages(messages);           \
    }
    FOR_EACH_MESSAGE_TYPE(DEFINE_SEND)
#undef DEFINE_SEND
  };

  class ServerSession : public RpcSession{
    friend class Server<RpcMessage, ServerSession>;
   private:
    UUID id_;

    void SetID(const UUID& id){
      id_ = id;
    }

#define DECLARE_HANDLER(Name) \
    void On##Name##Message(const Name##MessagePtr& msg);
    FOR_EACH_MESSAGE_TYPE(DECLARE_HANDLER)
#undef DECLARE_HANDLER
   public:
    ServerSession(uv_loop_t* loop):
      RpcSession(loop),
      id_(){
      handle_.data = this;
    }
    ~ServerSession() = default;

    UUID GetID() const{
      return id_;
    }
  };
}

#endif//TOKEN_SESSION_H