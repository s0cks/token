#ifndef TOKEN_RPC_MESSAGE_BLOCK_H
#define TOKEN_RPC_MESSAGE_BLOCK_H

#include "block.h"
#include "rpc/rpc_message_object.h"

namespace token{
  namespace rpc{
    class BlockMessage : public ObjectMessage<Block>{
    public:
    class Encoder : public codec::ObjectMessageEncoder<BlockMessage, Block::Encoder>{
      public:
        Encoder(const BlockMessage* value, const codec::EncoderFlags& flags):
          codec::ObjectMessageEncoder<BlockMessage, Block::Encoder>(value, flags){}
        ~Encoder() override = default;
      };

      class Decoder : public codec::ObjectMessageDecoder<BlockMessage, Block::Decoder>{
      public:
        explicit Decoder(const codec::DecoderHints& hints):
          codec::ObjectMessageDecoder<BlockMessage, Block::Decoder>(hints){}
        ~Decoder() override = default;
        BlockMessage* Decode(const BufferPtr& data) const override;
      };
     public:
      BlockMessage():
        ObjectMessage<Block>(){}
      explicit BlockMessage(const BlockPtr& val):
        ObjectMessage<Block>(val){}
      BlockMessage(const BlockMessage& other) = default;
      ~BlockMessage() override = default;

      Type type() const override{
        return Type::kBlockMessage;
      }

      int64_t GetBufferSize() const override{
        Encoder encoder(this, GetDefaultMessageEncoderFlags());
        return encoder.GetBufferSize();
      }

      bool Write(const BufferPtr& buff) const override{
        Encoder encoder(this, GetDefaultMessageEncoderFlags());
        return encoder.Encode(buff);
      }

      std::string ToString() const override{
        return "BlockMessage()";
      }

      BlockMessage& operator=(const BlockMessage& other) = default;

      static inline BlockMessagePtr
      NewInstance(const BlockPtr& val){
        return std::make_shared<BlockMessage>(val);
      }

      static BlockMessagePtr
      Decode(const BufferPtr& data, const codec::DecoderHints& hints=GetDefaultMessageDecoderHints()){
        Decoder decoder(hints);

        BlockMessage* value = nullptr;
        if(!(value = decoder.Decode(data))){
          DLOG(FATAL) << "cannot decode BlockMessage from buffer.";
          return nullptr;
        }
        return std::shared_ptr<BlockMessage>(value);
      }
    };
  }
}

#endif//TOKEN_RPC_MESSAGE_BLOCK_H