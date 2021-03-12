#include "rpc/rpc_message.h"

namespace token{
  int64_t PrepareMessage::GetMessageSize() const{
    return PaxosMessage::GetMessageSize();
  }

  bool PrepareMessage::WriteMessage(const BufferPtr& buff) const{
    return PaxosMessage::WriteMessage(buff);
  }

  int64_t PromiseMessage::GetMessageSize() const{
    return PaxosMessage::GetMessageSize();
  }

  bool PromiseMessage::WriteMessage(const BufferPtr& buff) const{
    return PaxosMessage::WriteMessage(buff);
  }

  int64_t CommitMessage::GetMessageSize() const{
    return PaxosMessage::GetMessageSize();
  }

  bool CommitMessage::WriteMessage(const BufferPtr& buff) const{
    return PaxosMessage::WriteMessage(buff);
  }

  int64_t AcceptedMessage::GetMessageSize() const{
    return PaxosMessage::GetMessageSize();
  }

  bool AcceptedMessage::WriteMessage(const BufferPtr& buff) const{
    return PaxosMessage::WriteMessage(buff);
  }

  int64_t RejectedMessage::GetMessageSize() const{
    return PaxosMessage::GetMessageSize();
  }

  bool RejectedMessage::WriteMessage(const BufferPtr& buff) const{
    return PaxosMessage::WriteMessage(buff);
  }
}