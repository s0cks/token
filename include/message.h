#ifndef TOKEN_MESSAGE_H
#define TOKEN_MESSAGE_H

#include <memory>
#include "object.h"

namespace token{
  class Message;
  typedef std::shared_ptr<Message> MessagePtr;
  typedef std::vector<MessagePtr> MessageList;

  class Message : public SerializableObject{
   protected:
    Message() = default;
   public:
    virtual ~Message() = default;
  };
};

#endif //TOKEN_MESSAGE_H