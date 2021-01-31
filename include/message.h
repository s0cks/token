#ifndef TOKEN_MESSAGE_H
#define TOKEN_MESSAGE_H

#include <memory>
#include "object.h"

namespace Token{
  class Message;
  typedef std::shared_ptr<Message> MessagePtr;
  typedef std::vector<MessagePtr> MessageList;

  class Message : public SerializableObject{
   protected:
    Message() = default;
   public:
    virtual ~Message() = default;
    virtual const char* GetName() const = 0;
  };
};

#endif //TOKEN_MESSAGE_H