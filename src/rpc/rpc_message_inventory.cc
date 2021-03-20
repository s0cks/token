#include "pool.h"
#include "rpc/rpc_message.h"

namespace token{
  int64_t InventoryMessage::GetMessageSize() const{
    int64_t size = 0;
    size += sizeof(int64_t); // length(items_)
    size += (items_.size() * InventoryItem::GetSize());
    return size;
  }

  bool InventoryMessage::WriteMessage(const BufferPtr& buff) const{
    return buff->PutSet(items());
  }

  int64_t NotFoundMessage::GetMessageSize() const{
    int64_t size = 0;
    size += sizeof(uint32_t);
    size += Hash::kSize;
    size += message_.length();
    return size;
  }

  bool NotFoundMessage::WriteMessage(const BufferPtr& buff) const{
    buff->PutUnsignedInt(static_cast<uint32_t>(item_.GetType()));
    buff->PutHash(item_.GetHash());
    buff->PutString(message_);
    return true;
  }
}