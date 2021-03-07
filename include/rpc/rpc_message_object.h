#ifndef TOKEN_RPC_MESSAGE_OBJECT_H
#define TOKEN_RPC_MESSAGE_OBJECT_H

#include "rpc/rpc_message.h"
#include "buffer.h"

namespace token{
  template<class T>
  class ObjectMessage : public RpcMessage{
   protected:
    std::shared_ptr<T> value_;

    ObjectMessage(const std::shared_ptr<T>& value):
      RpcMessage(),
      value_(value){}
    ObjectMessage(const BufferPtr& buff):
      RpcMessage(),
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
    std::string ToString() const{        \
      std::stringstream ss;              \
      ss << #Name << "Message(" << GetValue()->GetHash() << ")"; \
      return ss.str();                   \
    }                                    \
    bool Equals(const RpcMessagePtr& obj) const{                 \
      if(!obj->Is##Name##Message())      \
        return false;                    \
      return GetValue()->Equals(std::static_pointer_cast<Name##Message>(obj)->GetValue()); \
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

    static inline TransactionMessagePtr
    NewInstance(const TransactionPtr& tx){
      return std::make_shared<TransactionMessage>(tx);
    }

    static inline TransactionMessagePtr
    NewInstance(const BufferPtr& buff){
      return std::make_shared<TransactionMessage>(buff);
    }
  };

  class BlockMessage : public ObjectMessage<Block>{
   public:
    BlockMessage(const BlockPtr& blk):
      ObjectMessage(blk){}
    BlockMessage(const BufferPtr& buff):
      ObjectMessage(buff){}
    ~BlockMessage() = default;

    DEFINE_RPC_OBJECT_MESSAGE_TYPE(Block);

    static inline BlockMessagePtr
    NewInstance(const BlockPtr& blk){
      return std::make_shared<BlockMessage>(blk);
    }

    static inline BlockMessagePtr
    NewInstance(const BufferPtr& buff){
      return std::make_shared<BlockMessage>(buff);
    }
  };
}

#undef DEFINE_RPC_OBJECT_MESSAGE_TYPE

#endif//TOKEN_RPC_MESSAGE_OBJECT_H