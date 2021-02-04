#ifndef TOKEN_RPC_MESSAGE_INVENTORY_H
#define TOKEN_RPC_MESSAGE_INVENTORY_H

#include "rpc/rpc_message.h"

namespace token{
  class InventoryItem{
   public:
    enum Type{
      kUnknown = 0,
      kTransaction,
      kBlock,
      kUnclaimedTransaction
    };
   private:
    Type type_;
    Hash hash_;
   public:
    InventoryItem():
        type_(kUnknown),
        hash_(){}
    InventoryItem(Type type, const Hash& hash):
        type_(type),
        hash_(hash){}
    InventoryItem(const Transaction& tx):
        InventoryItem(kTransaction, tx.GetHash()){}
    InventoryItem(const BlockPtr& blk):
        InventoryItem(kBlock, blk->GetHash()){}
    InventoryItem(const UnclaimedTransactionPtr& utxo):
        InventoryItem(kUnclaimedTransaction, utxo->GetHash()){}
    InventoryItem(const BlockHeader& blk):
        InventoryItem(kBlock, blk.GetHash()){}
    InventoryItem(const InventoryItem& item):
        type_(item.type_),
        hash_(item.hash_){}
    InventoryItem(const BufferPtr& buff):
        type_(static_cast<Type>(buff->GetShort())),
        hash_(buff->GetHash()){}
    ~InventoryItem(){}

    Type GetType() const{
      return type_;
    }

    Hash GetHash() const{
      return hash_;
    }

    bool ItemExists() const;

    bool IsUnclaimedTransaction() const{
      return type_ == kUnclaimedTransaction;
    }

    bool IsBlock() const{
      return type_ == kBlock;
    }

    bool IsTransaction() const{
      return type_ == kTransaction;
    }

    bool Write(const BufferPtr& buff) const{
      buff->PutShort(static_cast<int16_t>(GetType()));
      buff->PutHash(GetHash());
      return true;
    }

    void operator=(const InventoryItem& other){
      type_ = other.type_;
      hash_ = other.hash_;
    }

    friend bool operator==(const InventoryItem& a, const InventoryItem& b){
      return a.type_ == b.type_ &&
          a.hash_ == b.hash_;
    }

    friend bool operator!=(const InventoryItem& a, const InventoryItem& b){
      return !operator==(a, b);
    }

    friend std::ostream& operator<<(std::ostream& stream, const InventoryItem& item){
      if(item.IsBlock()){
        stream << "Block(" << item.GetHash() << ")";
      } else if(item.IsTransaction()){
        stream << "Transaction(" << item.GetHash() << ")";
      } else if(item.IsUnclaimedTransaction()){
        stream << "UnclaimedTransaction(" << item.GetHash() << ")";
      } else{
        stream << "Unknown(" << item.GetHash() << ")";
      }
      return stream;
    }

    static inline int64_t
    GetSize(){
      int64_t size = 0;
      size += sizeof(int16_t);
      size += Hash::GetSize();
      return size;
    }
  };

  class InventoryMessage : public RpcMessage{
   public:
    static const size_t kMaxAmountOfItemsPerMessage = 50;
   protected:
    std::vector<InventoryItem> items_;
   public:
    InventoryMessage(const std::vector<InventoryItem>& items):
        RpcMessage(),
        items_(items){
      if(items_.empty())
        LOG(WARNING) << "inventory created w/ zero size";
    }
    InventoryMessage(const BufferPtr& buff):
        RpcMessage(),
        items_(){
      int64_t num_items = buff->GetLong();
      for(int64_t idx = 0; idx < num_items; idx++)
        items_.push_back(InventoryItem(buff));
    }
    ~InventoryMessage(){}

    int64_t GetNumberOfItems() const{
      return items_.size();
    }

    bool GetItems(std::vector<InventoryItem>& items){
      std::copy(items_.begin(), items_.end(), std::back_inserter(items));
      return items.size() == items_.size();
    }

    bool Equals(const RpcMessagePtr& obj) const{
      if(!obj->IsInventoryMessage()){
        return false;
      }
      InventoryMessagePtr msg = std::static_pointer_cast<InventoryMessage>(obj);
      return items_ == msg->items_;
    }

    std::string ToString() const{
      std::stringstream ss;
      ss << "InventoryMessage()"; //TODO: implement
      return ss.str();
    }

    DEFINE_RPC_MESSAGE(Inventory);

    static inline RpcMessagePtr
    NewInstance(const Hash& hash, const InventoryItem::Type& type){
      std::vector<InventoryItem> items = {
          InventoryItem(type, hash),
      };
      return NewInstance(items);
    }

    static inline RpcMessagePtr
    NewInstance(const BufferPtr& buff){
      int64_t num_items = buff->GetLong();
      std::vector<InventoryItem> items;
      for(int64_t idx = 0; idx < num_items; idx++)
        items.push_back(InventoryItem(buff));
      return std::make_shared<InventoryMessage>(items);
    }

    static inline RpcMessagePtr
    NewInstance(std::vector<InventoryItem>& items){
      return std::make_shared<InventoryMessage>(items);
    }

    static inline RpcMessagePtr
    NewInstance(const Transaction& tx){
      return NewInstance(tx.GetHash(), InventoryItem::kTransaction);
    }

    static inline RpcMessagePtr
    NewInstance(const BlockPtr& blk){
      return NewInstance(blk->GetHash(), InventoryItem::kBlock);
    }

    static inline RpcMessagePtr
    NewInstance(const BlockHeader& blk){
      return NewInstance(blk.GetHash(), InventoryItem::kBlock);
    }
  };

  class GetDataMessage : public RpcMessage{
   public:
    static const size_t kMaxAmountOfItemsPerMessage = 50;
   protected:
    std::vector<InventoryItem> items_;
   public:
    GetDataMessage(const std::vector<InventoryItem>& items):
        RpcMessage(),
        items_(items){
      if(items_.empty())
        LOG(WARNING) << "inventory created w/ zero size";
    }
    ~GetDataMessage() = default;

    int64_t GetNumberOfItems() const{
      return items_.size();
    }

    bool GetItems(std::vector<InventoryItem>& items){
      std::copy(items_.begin(), items_.end(), std::back_inserter(items));
      return items.size() == items_.size();
    }

    bool Equals(const RpcMessagePtr& obj) const{
      if(!obj->IsGetDataMessage())
        return false;
      GetDataMessagePtr msg = std::static_pointer_cast<GetDataMessage>(obj);
      return items_ == msg->items_;
    }

    std::string ToString() const{
      std::stringstream ss;
      ss << "GetDataMessage()"; //TODO: implement
      return ss.str();
    }

    DEFINE_RPC_MESSAGE(GetData);

    static inline RpcMessagePtr
    NewInstance(const BufferPtr& buff){
      int64_t num_items = buff->GetLong();
      std::vector<InventoryItem> items;
      for(int64_t idx = 0; idx < num_items; idx++)
        items.push_back(InventoryItem(buff));
      return std::make_shared<GetDataMessage>(items);
    }

    static inline RpcMessagePtr
    NewInstance(std::vector<InventoryItem>& items){
      return std::make_shared<GetDataMessage>(items);
    }

    static inline RpcMessagePtr
    NewInstance(const Transaction& tx){
      std::vector<InventoryItem> items = {
        InventoryItem(tx)
      };
      return NewInstance(items);
    }

    static inline RpcMessagePtr
    NewInstance(const BlockPtr& blk){
      std::vector<InventoryItem> items = {
        InventoryItem(blk)
      };
      return NewInstance(items);
    }
  };

  class NotFoundMessage : public RpcMessage{
   private:
    InventoryItem item_;
    std::string message_;
   public:
    NotFoundMessage(const std::string& message):
        RpcMessage(),
        message_(message){}
    ~NotFoundMessage() = default;

    std::string GetMessage() const{
      return message_;
    }

    bool Equals(const RpcMessagePtr& obj) const{
      if(!obj->IsNotFoundMessage())
        return false;
      NotFoundMessagePtr msg = std::static_pointer_cast<NotFoundMessage>(obj);
      return item_ == msg->item_;
    }

    std::string ToString() const{
      std::stringstream ss;
      ss << "NotFoundMessage()"; //TODO: implement
      return ss.str();
    }

    DEFINE_RPC_MESSAGE(NotFound);

    static inline RpcMessagePtr
    NewInstance(const BufferPtr& buff){
      std::string message = ""; //TODO: buff->GetString();
      return std::make_shared<NotFoundMessage>(message);
    }

    static inline RpcMessagePtr
    NewInstance(const std::string& message = "Not Found"){
      return std::make_shared<NotFoundMessage>(message);
    }
  };
}

#endif//TOKEN_RPC_MESSAGE_INVENTORY_H