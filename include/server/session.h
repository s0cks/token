#ifndef TOKEN_SESSION_H
#define TOKEN_SESSION_H

#include "message.h"
#include "server/channel.h"
#include "atomic/relaxed_atomic.h"

namespace Token{
#define FOR_EACH_SESSION_STATE(V) \
    V(Connecting)                 \
    V(Connected)                  \
    V(Disconnected)

  class Session{
    friend class Server; //TODO: revoke access
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
    Channel channel_;

    Channel& GetChannel(){
      return channel_;
    }

    void SetState(const State& state){
      state_ = state;
    }

    static void AllocBuffer(uv_handle_t* handle, size_t suggested_size, uv_buf_t* buf);
   public:
    Session(uv_loop_t* loop):
      state_(Session::kDisconnectedState),
      channel_(loop){}
    virtual ~Session() = default;

    State GetState() const{
      return state_;
    }

    inline void Send(const MessageList& messages){
      return GetChannel().Send(messages);
    }

    inline void Send(const MessagePtr& msg){
      MessageList messages = { msg };
      return GetChannel().Send(messages);
    }

#define DEFINE_SEND(Name) \
    inline void           \
    Send(const Name##MessagePtr& msg){ \
      MessageList messages = { msg };  \
      return Send(messages);           \
    }
    FOR_EACH_MESSAGE_TYPE(DEFINE_SEND)
#undef DEFINE_SEND

#define DEFINE_STATE_CHECK(Name) \
    inline bool Is##Name(){ return GetState() == Session::k##Name##State; }
    FOR_EACH_SESSION_STATE(DEFINE_STATE_CHECK)
#undef DEFINE_STATE_CHECK
  };

  class ServerSession : public Session{
    friend class Server;
   private:
    UUID id_;

    void SetID(const UUID& id){
      id_ = id;
    }

#define DEFINE_HANDLE_MESSAGE(Name) \
    static void Handle##Name##Message(ServerSession* session, const Name##MessagePtr& msg);
    FOR_EACH_MESSAGE_TYPE(DEFINE_HANDLE_MESSAGE)
#undef DEFINE_HANDLE_MESSAGE
   public:
    ServerSession(uv_loop_t* loop):
      Session(loop),
      id_(){}
    ~ServerSession() = default;

    UUID GetID() const{
      return id_;
    }
  };
}

#endif//TOKEN_SESSION_H