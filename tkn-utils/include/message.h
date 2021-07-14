#ifndef TOKEN_MESSAGE_H
#define TOKEN_MESSAGE_H

#include <memory>
#include <vector>

#include "object.h"

namespace token{
  class MessageBase;
  typedef std::shared_ptr<MessageBase> MessagePtr;
  typedef std::vector<MessagePtr> MessageList;

  static inline MessageList&
  operator<<(MessageList& messages, const MessagePtr& msg){
    messages.push_back(msg);
    return messages;
  }

  class MessageBase : public Object{
   protected:
    MessageBase() = default;
   public:
    ~MessageBase() override = default;
    virtual int64_t GetBufferSize() const = 0;
    virtual bool Write(const BufferPtr& buff) const = 0;
  };
}

#endif //TOKEN_MESSAGE_H