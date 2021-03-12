#include "pool.h"
#include "rpc/rpc_message.h"

namespace token{
  bool InventoryItem::ItemExists() const{
    switch(type_){
      case kTransaction: return ObjectPool::HasTransaction(hash_);
      case kBlock:{
        if(BlockChain::HasBlock(hash_)){
          LOG(INFO) << hash_ << " was found in the blockchain.";
          return true;
        } else if(ObjectPool::HasBlock(hash_)){
          LOG(INFO) << hash_ << " was found in the object pool.";
          return true;
        }
        LOG(WARNING) << "block " << hash_ << " was not found";
        return false;
      }
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
    return buff->PutVector(items_);
  }

  bool GetDataMessage::WriteMessage(const BufferPtr& buff) const{
    return buff->PutVector(items_);
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