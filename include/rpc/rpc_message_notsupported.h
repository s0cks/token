#ifndef TOKEN_RPC_MESSAGE_NOTSUPPORTED_H
#define TOKEN_RPC_MESSAGE_NOTSUPPORTED_H

#include <string>

namespace token{
  class NotSupportedMessage : public RpcMessage{
   private:
    std::string message_;
   public:
    NotSupportedMessage(const std::string& message=""):
      RpcMessage(),
      message_(message){}
    ~NotSupportedMessage() = default;

    std::string GetMessage() const{
      return message_;
    }

    bool Equals(const RpcMessagePtr& obj) const{
      if(!obj->IsNotSupportedMessage())
        return false;
      NotSupportedMessagePtr msg = std::static_pointer_cast<NotSupportedMessage>(obj);
      return GetMessage() == msg->GetMessage();
    }

    std::string ToString() const{
      std::stringstream ss;
      ss << "NotSupported(message=" << message_ << ")";
      return ss.str();
    }

    DEFINE_RPC_MESSAGE(NotSupported);

    static inline NotSupportedMessagePtr
    NewInstance(const std::string& message=""){
      return std::make_shared<NotSupportedMessage>(message);
    }

    static inline NotSupportedMessagePtr
    NewInstance(const BufferPtr& buff){
      int64_t size = buff->GetLong();
      std::string message = buff->GetString(size);
      return NewInstance(message);
    }
  };
}

#endif//TOKEN_RPC_MESSAGE_NOTSUPPORTED_H