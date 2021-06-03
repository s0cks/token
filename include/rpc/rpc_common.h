#ifndef TOKEN_RPC_COMMON_H
#define TOKEN_RPC_COMMON_H

#include <vector>
#include <memory>

namespace token{
  namespace rpc{
    class Message;
    typedef std::shared_ptr<Message> MessagePtr;
    typedef std::vector<MessagePtr> MessageList;

    class Session;
    typedef std::shared_ptr<Session> SessionPtr;
  }
}

#endif//TOKEN_RPC_COMMON_H