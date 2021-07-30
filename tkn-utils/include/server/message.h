#ifndef TOKEN_MESSAGE_H
#define TOKEN_MESSAGE_H

#include <memory>
#include <vector>

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
    virtual std::string ToString() const = 0;
    virtual int64_t GetBufferSize() const = 0;

    virtual internal::BufferPtr ToBuffer() const{
      return nullptr;
    }

    virtual bool Write(const internal::BufferPtr& buff) const = 0;
  };
}

#endif //TOKEN_MESSAGE_H