#include "runtime.h"
#include "node/node_server.h"

namespace token{
  namespace node{
    Server::Server(Runtime* runtime):
      ServerBase<Session>(runtime->loop()),
      runtime_(runtime){}
  }
}