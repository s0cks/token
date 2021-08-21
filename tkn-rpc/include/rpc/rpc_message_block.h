#ifndef TOKEN_RPC_MESSAGE_BLOCK_H
#define TOKEN_RPC_MESSAGE_BLOCK_H

#include "block.h"
#include "rpc/rpc_message_object.h"

namespace token{
  namespace rpc{
    class BlockMessage : public RawMessage<internal::proto::Block>{
    public:
      BlockMessage():
        RawMessage<internal::proto::Block>(){}
      explicit BlockMessage(internal::proto::Block raw):
        RawMessage<internal::proto::Block>(std::move(raw)){}
      explicit BlockMessage(const internal::BufferPtr& data, const uint64_t& msize):
        RawMessage<internal::proto::Block>(data, msize){}
      ~BlockMessage() override = default;

      Type type() const override{
        return Type::kBlockMessage;
      }

      std::string ToString() const override{
        return "BlockMessage()";
      }

      BlockPtr value() const{
        return Block::From(raw_);
      }

      static inline BlockMessagePtr
      NewInstance(const BlockPtr& data){
        RawBlock raw;
        raw << data;
        return std::make_shared<BlockMessage>(raw);
      }

      static inline BlockMessagePtr
      Decode(const BufferPtr& data, const uint64_t& msize){
        return std::make_shared<BlockMessage>(data, msize);
      }
    };
  }
}

#endif//TOKEN_RPC_MESSAGE_BLOCK_H