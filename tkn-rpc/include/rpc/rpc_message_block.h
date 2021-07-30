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
      explicit BlockMessage(const internal::BufferPtr& data):
        RawMessage<internal::proto::Block>(){
        if(!raw_.ParseFromArray(data->data(), static_cast<int>(data->length())))
          DLOG(FATAL) << "cannot parse BlockMessage from buffer.";
      }
      ~BlockMessage() override = default;

      Type type() const override{
        return Type::kBlockMessage;
      }

      int64_t GetBufferSize() const override{
        return static_cast<int64_t>(raw_.ByteSizeLong());
      }

      bool Write(const BufferPtr& data) const override{
        return raw_.SerializeToArray(data->data(), static_cast<int>(data->length()));
      }

      std::string ToString() const override{
        return "BlockMessage()";
      }

      BlockPtr value() const{
        return Block::NewInstance(raw_);
      }

      static inline BlockMessagePtr
      NewInstance(const BlockPtr& data){
        return std::make_shared<BlockMessage>(data->raw_);
      }

      static inline BlockMessagePtr
      Decode(const BufferPtr& data, const codec::DecoderHints& hints=codec::kDefaultDecoderHints){
        return std::make_shared<BlockMessage>(data);
      }
    };
  }
}

#endif//TOKEN_RPC_MESSAGE_BLOCK_H