#ifndef TOKEN_RPC_MESSAGE_INVENTORY_H
#define TOKEN_RPC_MESSAGE_INVENTORY_H

#include "rpc/rpc_message.h"
#include "inventory.h"

namespace token{
  namespace rpc{
    class InventoryMessage: public rpc::Message{
    public:
      static const size_t kMaxAmountOfItemsPerMessage = 50;
    protected:
      InventoryItems items_;

      explicit InventoryMessage(const InventoryItems& items):
         rpc::Message(),
         items_(items.begin(), items.end()){
        if(items_.empty())
          LOG(WARNING) << "inventory create w/ zero size";
        if(items_.size() != items.size())
          LOG(WARNING) << "inventory wasn't fully created.";
      }

      explicit InventoryMessage(const BufferPtr& buff):
         rpc::Message(),
         items_(){
        if(!Decode(buff, items_))
          LOG(WARNING) << "couldn't decode items from buffer.";
      }

      static inline bool
      CompareInventories(const InventoryItems& a, const InventoryItems& b){
        return std::equal(a.begin(), a.end(),
                          b.begin(),
                          [](const InventoryItem& a, const InventoryItem& b){
                            return InventoryItem::Compare(a, b) == 0;
                          });
      }

    public:
      virtual ~InventoryMessage() = default;

      int64_t GetMessageSize() const{
        return Buffer::CalculateSizeOf(items_);
      }

      bool WriteMessage(const BufferPtr& buffer) const{
        return buffer->PutSet(items_);
      }

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

    class InventoryListMessage: public InventoryMessage{
    public:
      explicit InventoryListMessage(const InventoryItems& items):
         InventoryMessage(items){}

      explicit InventoryListMessage(const BufferPtr& buffer):
         InventoryMessage(buffer){}

      ~InventoryListMessage() override = default;

      DEFINE_RPC_MESSAGE_TYPE(InventoryList);

      bool Equals(const rpc::MessagePtr& other) const override{
        NOT_IMPLEMENTED(ERROR);
        return false;
      }

      std::string ToString() const override{
        std::stringstream ss;
        ss << "InventoryListMessage(";
        ss << "items=" << items_;
        ss << ")";
        return ss.str();
      }

      static inline rpc::InventoryListMessagePtr
      NewInstance(const InventoryItems& items){
        return std::make_shared<InventoryListMessage>(items);
      }

      static inline rpc::InventoryListMessagePtr
      NewInstance(const std::vector<InventoryItem>& items){
        return NewInstance(InventoryItems(items.begin(), items.end()));
      }

      static inline rpc::InventoryListMessagePtr
      NewInstance(const BufferPtr& buff){
        int64_t num_items = buff->GetLong();
        InventoryItems items;
        for(int64_t idx = 0; idx < num_items; idx++)
          items.insert(InventoryItem(buff));
        return std::make_shared<InventoryListMessage>(items);
      }

      static inline rpc::InventoryListMessagePtr
      Of(const TransactionPtr& tx){
        InventoryItems items = {InventoryItem::Of(tx)};
        return NewInstance(items);
      }

      static inline rpc::InventoryListMessagePtr
      Of(const BlockHeader& blk){
        InventoryItems items = {InventoryItem::Of(blk)};
        return NewInstance(items);
      }

      static inline rpc::InventoryListMessagePtr
      Of(const BlockPtr& blk){
        InventoryItems items = {InventoryItem::Of(blk)};
        return NewInstance(items);
      }
    };

    static inline rpc::MessageList
    operator<<(rpc::MessageList& list, const rpc::InventoryListMessagePtr& msg){
      list.push_back(msg);
      return list;
    }

    class GetDataMessage: public InventoryMessage{
    public:
      explicit GetDataMessage(const InventoryItems& items):
         InventoryMessage(items){}

      explicit GetDataMessage(const BufferPtr& buffer):
         InventoryMessage(buffer){}

      ~GetDataMessage() override = default;

      DEFINE_RPC_MESSAGE_TYPE(GetData);

      bool Equals(const rpc::MessagePtr& obj) const override{
        if(!obj->IsGetDataMessage())
          return false;
        auto msg = std::static_pointer_cast<GetDataMessage>(obj);
        return std::equal(items_begin(),
                          items_end(),
                          msg->items_begin(),
                          [](const InventoryItem& a, const InventoryItem& b){
                            return a == b;
                          });
      }

      std::string ToString() const override{
        std::stringstream ss;
        ss << "GetDataMessage(";
        ss << "items=" << items_;
        ss << ")";
        return ss.str();
      }

      static inline rpc::GetDataMessagePtr
      NewInstance(const InventoryItems& items){
        return std::make_shared<GetDataMessage>(items);
      }

      static inline rpc::GetDataMessagePtr
      NewInstance(const BufferPtr& buff){
        InventoryItems items;
        if(!buff->GetSet(items))
          return nullptr;
        return std::make_shared<GetDataMessage>(items);
      }

      static inline rpc::GetDataMessagePtr
      ForBlock(const Hash& hash){
        InventoryItems items = {InventoryItem(Type::kBlock, hash)};
        return NewInstance(items);
      }

      static inline rpc::GetDataMessagePtr
      ForTransaction(const Hash& hash){
        InventoryItems items = {InventoryItem(Type::kTransaction, hash)};
        return NewInstance(items);
      }
    };

    static inline rpc::MessageList
    operator<<(rpc::MessageList& list, const rpc::GetDataMessagePtr& msg){
      list.push_back(msg);
      return list;
    }

    class NotFoundMessage: public rpc::Message{
    private:
      InventoryItem item_;
      std::string message_;
    public:
      explicit NotFoundMessage(const InventoryItem& item, std::string message):
         rpc::Message(),
         item_(item),
         message_(std::move(message)){}

      ~NotFoundMessage() override = default;

      DEFINE_RPC_MESSAGE_TYPE(NotFound);

      std::string GetMessage() const{
        return message_;
      }

      int64_t GetMessageSize() const override{
        int64_t size = 0;
        size += InventoryItem::GetSize();
        size += Buffer::CalculateSizeOf(message_);
        return size;
      }

      bool WriteMessage(const BufferPtr& buff) const override{
        return item_.Write(buff)
           && buff->PutLong(message_.size())
           && buff->PutString(message_);
      }

      bool Equals(const rpc::MessagePtr& obj) const override{
        if(!obj->IsNotFoundMessage())
          return false;
        auto msg = std::static_pointer_cast<NotFoundMessage>(obj);
        return item_ == msg->item_;
      }

      std::string ToString() const override{
        std::stringstream ss;
        ss << "NotFoundMessage(";
        ss << "message=" << message_;
        ss << ")";
        return ss.str();
      }

      static inline rpc::NotFoundMessagePtr
      NewInstance(const BufferPtr& buff){
        InventoryItem item = InventoryItem(buff);
        int64_t length = buff->GetLong();
        std::string message = buff->GetString(length);
        return std::make_shared<NotFoundMessage>(item, message);
      }

      static inline rpc::NotFoundMessagePtr
      NewInstance(const InventoryItem& item, const std::string& message = "Not Found"){
        return std::make_shared<NotFoundMessage>(item, message);
      }
    };

    static inline rpc::MessageList&
    operator<<(rpc::MessageList& list, const rpc::NotFoundMessagePtr& msg){
      list.push_back(msg);
      return list;
    }
  }
}
#endif//TOKEN_RPC_MESSAGE_INVENTORY_H