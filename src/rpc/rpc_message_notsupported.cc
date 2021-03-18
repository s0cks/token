#include "rpc/rpc_message.h"

namespace token{
  int64_t NotSupportedMessage::GetMessageSize() const{
    int64_t size = 0;
    size += sizeof(int64_t);
    size += message_.length();
    return size;
  }

  bool NotSupportedMessage::WriteMessage(const BufferPtr& buff) const{
    return buff->PutLong(message_.length())
        && buff->PutString(message_);
  }
}