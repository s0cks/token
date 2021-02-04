#include "pool.h"
#include "rpc/rpc_message_inventory.h"

namespace token{
  bool InventoryItem::ItemExists() const{
    switch(type_){
      case kTransaction: return ObjectPool::HasTransaction(hash_);
      case kBlock: return BlockChain::HasBlock(hash_) || ObjectPool::HasBlock(hash_);
      case kUnclaimedTransaction: return ObjectPool::HasUnclaimedTransaction(hash_);
      default: return false;
    }
  }

  int64_t InventoryMessage::GetMessageSize() const{
    int64_t size = 0;
    size += sizeof(int64_t); // length(items_)
    size += (items_.size() * InventoryItem::GetSize());
    return size;
  }

  bool InventoryMessage::WriteMessage(const BufferPtr& buff) const{
    buff->PutList(items_);
    return true;
  }

  bool GetDataMessage::WriteMessage(const BufferPtr& buff) const{
    buff->PutList(items_);
    return true;
  }

  int64_t GetDataMessage::GetMessageSize() const{
    int64_t size = 0;
    size += sizeof(int64_t); // sizeof(items_);
    size += items_.size() * InventoryItem::GetSize(); // items;
    return size;
  }

  int64_t NotFoundMessage::GetMessageSize() const{
    int64_t size = 0;
    size += sizeof(int32_t);
    size += Hash::kSize;
    size += message_.length();
    return size;
  }

  bool NotFoundMessage::WriteMessage(const BufferPtr& buff) const{
    buff->PutInt(static_cast<int32_t>(item_.GetType()));
    buff->PutHash(item_.GetHash());
    buff->PutString(message_);
    return true;
  }
}