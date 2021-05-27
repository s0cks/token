#ifndef TOKEN_RPC_MESSAGE_OBJECT_H
#define TOKEN_RPC_MESSAGE_OBJECT_H

#include "buffer.h"

namespace token{
  namespace rpc{
    template<class T>
    class ObjectMessage : public rpc::Message{
    protected:
      std::shared_ptr<T> value_;

      ObjectMessage(const std::shared_ptr<T>& value):
         rpc::Message(),
         value_(value){}
      ObjectMessage(const BufferPtr& buff):
         rpc::Message(),
         value_(T::FromBytes(buff)){}

      int64_t GetMessageSize() const{
        return value_->GetBufferSize();
      }

      bool WriteMessage(const BufferPtr& buffer) const{
        return value_->Write(buffer);
      }
    public:
      virtual ~ObjectMessage() = default;

      std::shared_ptr<T> GetValue() const{
        return value_;
      }
    };

#define DEFINE_RPC_OBJECT_MESSAGE_TYPE(Name) \
    std::string ToString() const override{        \
      std::stringstream ss;              \
      ss << #Name << "Message(" << GetValue()->GetHash() << ")"; \
      return ss.str();                   \
    }                                    \
    bool Equals(const rpc::MessagePtr& obj) const override{                 \
      if(!obj->Is##Name##Message())      \
        return false;                    \
      return GetValue()->Equals(std::static_pointer_cast<rpc::Name##Message>(obj)->GetValue()); \
    }                                        \
    DEFINE_RPC_MESSAGE_TYPE(Name)

    class TransactionMessage : public ObjectMessage<Transaction>{
    public:
      TransactionMessage(const TransactionPtr& value):
         ObjectMessage(value){}
      TransactionMessage(const BufferPtr& buff):
         ObjectMessage(buff){}
      ~TransactionMessage() = default;

      DEFINE_RPC_OBJECT_MESSAGE_TYPE(Transaction);

      static inline rpc::TransactionMessagePtr
      NewInstance(const TransactionPtr& tx){
        return std::make_shared<rpc::TransactionMessage>(tx);
      }

      static inline rpc::TransactionMessagePtr
      NewInstance(const BufferPtr& buff){
        return std::make_shared<rpc::TransactionMessage>(buff);
      }
    };

    static inline rpc::MessageList&
    operator<<(rpc::MessageList& list, const rpc::TransactionMessagePtr& msg){
      list.push_back(msg);
      return list;
    }

    class BlockMessage : public ObjectMessage<Block>{
    public:
      BlockMessage(const BlockPtr& blk):
         ObjectMessage(blk){}
      BlockMessage(const BufferPtr& buff):
         ObjectMessage(buff){}
      ~BlockMessage() override = default;

      DEFINE_RPC_OBJECT_MESSAGE_TYPE(Block);

      static inline rpc::BlockMessagePtr
      NewInstance(const BlockPtr& blk){
        return std::make_shared<rpc::BlockMessage>(blk);
      }

      static inline rpc::BlockMessagePtr
      NewInstance(const BufferPtr& buff){
        return std::make_shared<rpc::BlockMessage>(buff);
      }
    };

    static inline rpc::MessageList&
    operator<<(rpc::MessageList& list, const rpc::BlockMessagePtr& msg){
      list.push_back(msg);
      return list;
    }
  }
}

#undef DEFINE_RPC_OBJECT_MESSAGE_TYPE

#endif//TOKEN_RPC_MESSAGE_OBJECT_H