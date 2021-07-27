#ifndef TOKEN_RPC_MESSAGE_BLOCK_H
#define TOKEN_RPC_MESSAGE_BLOCK_H

#include "block.h"
#include "rpc/rpc_message_object.h"

namespace token{
  namespace rpc{
    class BlockMessage : public ObjectMessage<Block>{
    public:
      class Encoder : public ObjectMessageEncoder<rpc::BlockMessage, BlockEncoder>{
      public:
        Encoder(const rpc::BlockMessage& value, const codec::EncoderFlags& flags):
          ObjectMessageEncoder<rpc::BlockMessage, >(value, flags){}
        Encoder(const Encoder& other) = default;
        ~Encoder() override = default;
        Encoder& operator=(const Encoder& other) = delete;//TODO: don't delete this copy assignment operator
      };

      class Decoder : public ObjectMessageDecoder<rpc::BlockMessage, BlockDecoder>{
      public:
        explicit Decoder(const DecoderHints& hints):
            ObjectMessageDecoder<rpc::BlockMessage, BlockDecoder>(hints){}
        Decoder(const Decoder& rhs) = default;
        ~Decoder() override = default;
        bool Decode(const BufferPtr& buff, rpc::BlockMessage& result) const override;
        Decoder& operator=(const Decoder& rhs) = default;
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
        codec::Encoder encoder((*this), GetDefaultMessageEncoderFlags());
        return encoder.GetBufferSize();
      }

      bool Write(const BufferPtr& buff) const override{
        codec::Encoder encoder((*this), GetDefaultMessageEncoderFlags());
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

      static inline bool
      Decode(const BufferPtr& buff, BlockMessage& result, const codec::DecoderHints& hints=codec::kDefaultDecoderHints){
        codec::Decoder decoder(hints);
        return decoder.Decode(buff, result);
      }

      static inline BlockMessagePtr
      DecodeNew(const BufferPtr& buff, const codec::DecoderHints& hints=codec::kDefaultDecoderHints){
        BlockMessage instance;
        if(!Decode(buff, instance, hints)){
          DLOG(ERROR) << "cannot decode BlockMessage.";
          return nullptr;
        }
        return std::make_shared<BlockMessage>(instance);
      }
    };
  }
}

#endif//TOKEN_RPC_MESSAGE_BLOCK_H