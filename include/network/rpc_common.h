#ifndef TOKEN_RPC_COMMON_H
#define TOKEN_RPC_COMMON_H

#include <vector>
#include <memory>

namespace token{
  namespace rpc{
    //TODO:
    // - refactor this
    enum class ClientType{
      kUnknown = 0,
      kNode,
      kClient
    };

    static std::ostream& operator<<(std::ostream& stream, const ClientType& type){
      switch (type){
        case ClientType::kNode:return stream << "Node";
        case ClientType::kClient:return stream << "Client";
        case ClientType::kUnknown:
        default:return stream << "Unknown";
      }
    }

    class Message;
    typedef std::shared_ptr<Message> MessagePtr;
    typedef std::vector<MessagePtr> MessageList;

    class Session;
    typedef std::shared_ptr<Session> SessionPtr;
  }
}

#endif//TOKEN_RPC_COMMON_H