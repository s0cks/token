#ifndef TOKEN_SERVER_SESSION_H
#define TOKEN_SERVER_SESSION_H

#include "session.h"

namespace Token{
  class ServerSession : public Session{
    friend class Server;
   private:
    UUID uuid_;

    void SetID(const UUID& uuid){
      uuid_ = uuid;
    }

#define DECLARE_MESSAGE_HANDLER(Name) \
      static void Handle##Name##Message(ServerSession*, const Name##MessagePtr& msg);
    FOR_EACH_MESSAGE_TYPE(DECLARE_MESSAGE_HANDLER)
#undef DECLARE_MESSAGE_HANDLER
   public:
    ServerSession(uv_loop_t* loop):
      Session(loop),
      uuid_(){}
    ~ServerSession() = default;

    UUID GetID() const{
      return uuid_;
    }

    std::string ToString() const{
      std::stringstream ss;
      ss << "ServerSession(" << GetID() << ")";
      return ss.str();
    }

    static ServerSession* NewInstance(uv_loop_t* loop){
      return new ServerSession(loop);
    }
  };
}

#endif //TOKEN_SERVER_SESSION_H