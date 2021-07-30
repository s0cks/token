#ifndef TKN_NODE_SERVER_H
#define TKN_NODE_SERVER_H

#include "server/server.h"
#include "node/node_session.h"

namespace token{
  class Runtime;

  namespace node{
    class Server : public ServerBase<Session>{
    protected:
      Runtime* runtime_;

      Session* CreateSession() override{
        return new Session(this);
      }
    public:
      explicit Server(Runtime* runtime);
      ~Server() override = default;

      Runtime* runtime() const{
        return runtime_;
      }
    };
  }
}


#endif//TKN_NODE_SERVER_H