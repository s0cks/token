#include "rpc/rpc_message.h"

namespace token{
  const int64_t GetBlocksMessage::kMaxNumberOfBlocks = 32;

  GetBlocksMessagePtr GetBlocksMessage::NewInstance(const BufferPtr& buff){
    Hash start = buff->GetHash();
    Hash stop = buff->GetHash();
    return std::make_shared<GetBlocksMessage>(start, stop);
  }

  bool GetBlocksMessage::WriteMessage(const BufferPtr& buff) const{
    buff->PutHash(start_);
    buff->PutHash(stop_);
    return true;
  }

  int64_t GetBlocksMessage::GetMessageSize() const{
    return Hash::GetSize() * 2;
  }
}