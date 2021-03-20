#ifndef TOKEN_RPC_MESSAGE_INVENTORY_H
#define TOKEN_RPC_MESSAGE_INVENTORY_H

#include "inventory.h"

namespace token{
  class InventoryMessage : public RpcMessage{
   public:
    static const size_t kMaxAmountOfItemsPerMessage = 50;
   protected:
    InventoryItems items_;

    InventoryMessage(const InventoryItems& items):
      RpcMessage(),
      items_(items.begin(), items.end()){
      if(items_.empty())
        LOG(WARNING) << "inventory create w/ zero size";
      if(items_.size() != items.size())
        LOG(WARNING) << "inventory wasn't fully created.";
    }
    InventoryMessage(const BufferPtr& buff):
      RpcMessage(),
      items_(){
      if(!Decode(buff, items_))
        LOG(WARNING) << "couldn't decode items from buffer.";
    }
   public:
    virtual ~InventoryMessage() = default;

    int64_t GetMessageSize() const;
    bool WriteMessage(const BufferPtr& buffer) const;

    int64_t GetNumberOfItems() const{
      return items_.size();
    }

    InventoryItems& items(){
      return items_;
    }

    InventoryItems items() const{
      return items_;
    }

    InventoryItems::iterator items_begin(){
      return items_.begin();
    }

    InventoryItems::const_iterator items_begin() const{
      return items_.begin();
    }

    InventoryItems::iterator items_end(){
      return items_.end();
    }

    InventoryItems::const_iterator items_end() const{
      return items_.end();
    }
  };

  class InventoryListMessage : public InventoryMessage{
   public:
    InventoryListMessage(const InventoryItems& items):
      InventoryMessage(items){}
    InventoryListMessage(const BufferPtr& buffer):
      InventoryMessage(buffer){}
    ~InventoryListMessage() = default;

    bool Equals(const RpcMessagePtr& other) const{
      InventoryItems items;
      InventoryListMessage m(items);
      return false;
    }

    std::string ToString() const{
      std::stringstream ss;
      ss << "InventoryListMessage(";
      ss << "items=" << items();
      ss << ")";
      return ss.str();
    }

    DEFINE_RPC_MESSAGE_TYPE(InventoryList);

    static inline InventoryListMessagePtr
    NewInstance(const InventoryItems& items){
      return std::make_shared<InventoryListMessage>(items);
    }

    static inline InventoryListMessagePtr
    NewInstance(const std::vector<InventoryItem>& items){
      return NewInstance(InventoryItems(items.begin(), items.end()));
    }

    static inline InventoryListMessagePtr
    NewInstance(const BufferPtr& buff){
      int64_t num_items = buff->GetLong();
      InventoryItems items;
      for(int64_t idx = 0; idx < num_items; idx++)
        items.insert(InventoryItem(buff));
      return std::make_shared<InventoryListMessage>(items);
    }

    static inline InventoryListMessagePtr
    Of(const TransactionPtr& tx){
      InventoryItems items = { InventoryItem::Of(tx) };
      return NewInstance(items);
    }

    static inline InventoryListMessagePtr
    Of(const BlockHeader& blk){
      InventoryItems items = { InventoryItem::Of(blk) };
      return NewInstance(items);
    }

    static inline InventoryListMessagePtr
    Of(const BlockPtr& blk){
      InventoryItems items = { InventoryItem::Of(blk) };
      return NewInstance(items);
    }
  };

  class GetDataMessage : public InventoryMessage{
   public:
    GetDataMessage(const InventoryItems& items):
      InventoryMessage(items){}
    GetDataMessage(const BufferPtr& buffer):
      InventoryMessage(buffer){}
    ~GetDataMessage() = default;

    bool Equals(const RpcMessagePtr& obj) const{
      //TODO: refactor
      if(!obj->IsGetDataMessage())
        return false;
      GetDataMessagePtr msg = std::static_pointer_cast<GetDataMessage>(obj);
      return items_ == msg->items_;
    }

    std::string ToString() const{
      std::stringstream ss;
      ss << "GetDataMessage(";
      ss << "items=" << items();
      ss << ")";
      return ss.str();
    }

    DEFINE_RPC_MESSAGE_TYPE(GetData);

    static inline GetDataMessagePtr
    NewInstance(const InventoryItems& items){
      return std::make_shared<GetDataMessage>(items);
    }

    static inline GetDataMessagePtr
    NewInstance(const std::vector<InventoryItem>& items){
      return NewInstance(InventoryItems(items.begin(), items.end()));
    }

    static inline GetDataMessagePtr
    NewInstance(const BufferPtr& buff){
      int64_t num_items = buff->GetLong();
      InventoryItems items;
      for(int64_t idx = 0; idx < num_items; idx++)
        items.insert(InventoryItem(buff));
      return std::make_shared<GetDataMessage>(items);
    }

    static inline GetDataMessagePtr
    ForBlock(const Hash& hash){
      InventoryItems items = { InventoryItem(Type::kBlock, hash) };
      return NewInstance(items);
    }

    static inline GetDataMessagePtr
    ForTransaction(const Hash& hash){
      InventoryItems items = { InventoryItem(Type::kTransaction, hash) };
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

    static inline NotFoundMessagePtr
    NewInstance(const BufferPtr& buff){
      std::string message = ""; //TODO: buff->GetString();
      return std::make_shared<NotFoundMessage>(message);
    }

    static inline NotFoundMessagePtr
    NewInstance(const std::string& message = "Not Found"){
      return std::make_shared<NotFoundMessage>(message);
    }
  };
}

#endif//TOKEN_RPC_MESSAGE_INVENTORY_H