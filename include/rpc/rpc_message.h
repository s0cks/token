#ifndef TOKEN_RPC_MESSAGE_H
#define TOKEN_RPC_MESSAGE_H

#include <set>
#include <memory>
#include <vector>
#include "buffer.h"
#include "message.h"
#include "rpc/rpc_common.h"

namespace token{
  namespace rpc{
    class Message: public MessageBase{
    protected:
      Message() = default;
      virtual int64_t GetMessageSize() const = 0;
      virtual bool WriteMessage(const BufferPtr& buff) const = 0;
    public:
      ~Message() override = default;

      int64_t GetBufferSize() const override{
        int64_t size = 0;
        size += sizeof(RawObjectTag);
        size += GetMessageSize();
        return size;
      }

      bool Write(const BufferPtr& buff) const override;
      virtual bool Equals(const rpc::MessagePtr& msg) const = 0;

      static MessagePtr From(const BufferPtr& buffer);
    };

#define DEFINE_RPC_MESSAGE_TYPE(Name) \
  Type GetType() const override{ return Type::k##Name##Message; }

#define DEFINE_RPC_MESSAGE(Name) \
  DEFINE_RPC_MESSAGE_TYPE(Name) \
  int64_t GetMessageSize() const override; \
  bool WriteMessage(const BufferPtr& buff) const override;

#define DEFINE_RPC_MESSAGE_CONSTRUCTORS(Name) \
  static inline Name##MessagePtr NewInstance(const BufferPtr& buffer){ return std::make_shared<Name##Message>(buffer); }
  }
}

#endif//TOKEN_RPC_MESSAGE_H