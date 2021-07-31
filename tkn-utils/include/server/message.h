#ifndef TOKEN_MESSAGE_H
#define TOKEN_MESSAGE_H

#include <memory>
#include <vector>

#include "type.h"
#include "buffer.h"

namespace token{
  class MessageBase;
  typedef std::shared_ptr<MessageBase> MessagePtr;
  typedef std::vector<MessagePtr> MessageList;

  static inline MessageList&
  operator<<(MessageList& messages, const MessagePtr& msg){
    messages.push_back(msg);
    return messages;
  }

  class MessageBase{ //TODO: extend Object
   protected:
    MessageBase() = default;
   public:
    virtual ~MessageBase() = default;
    virtual Type type() const = 0;
    virtual uint64_t GetBufferSize() const = 0;//TODO: remove
    virtual internal::BufferPtr ToBuffer() const = 0;
    virtual std::string ToString() const = 0;
  };
}

#endif //TOKEN_MESSAGE_H