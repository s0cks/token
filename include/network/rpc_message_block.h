#ifndef TOKEN_RPC_MESSAGE_BLOCK_H
#define TOKEN_RPC_MESSAGE_BLOCK_H

#include "network/rpc_message_object.h"

namespace token{
  namespace rpc{
    class BlockMessage : public ObjectMessage<Block>{
     public:
      class Encoder : public ObjectMessageEncoder<BlockMessage>{
       public:
        Encoder(const BlockMessage& value, const codec::EncoderFlags& flags=codec::kDefaultEncoderFlags):
          ObjectMessageEncoder<BlockMessage>(value, flags){}
        Encoder(const Encoder& other) = default;
        ~Encoder() override = default;
        int64_t GetBufferSize() const override;
        bool Encode(const BufferPtr& buff) const override;
        Encoder& operator=(const Encoder& other) = delete;//TODO: don't delete this copy assignment operator
      };

      class Decoder : public ObjectMessageDecoder<BlockMessage>{
       public:
        Decoder(const codec::DecoderHints& hints=codec::kDefaultDecoderHints):
          ObjectMessageDecoder<BlockMessage>(hints){}
        Decoder(const Decoder& other) = default;
        ~Decoder() override = default;
        bool Decode(const BufferPtr& buff, BlockMessage& result) const override;
        Decoder& operator=(const Decoder& other) = default;
      };
     public:
      BlockMessage():
        ObjectMessage<Block>(){}
      BlockMessage(const BlockPtr& val):
        ObjectMessage<Block>(val){}
      BlockMessage(const BlockMessage& other) = default;
      ~BlockMessage() override = default;

      Type type() const override{
        return Type::kBlockMessage;
      }

      int64_t GetBufferSize() const override{
        Encoder encoder((*this));
        return encoder.GetBufferSize();
      }

      bool Write(const BufferPtr& buff) const override{
        Encoder encoder((*this));
        return encoder.Encode(buff);
      }

      std::string ToString() const override;
      BlockMessage& operator=(const BlockMessage& other) = default;

      static inline BlockMessagePtr
      NewInstance(const BlockPtr& val){
        return std::make_shared<BlockMessage>(val);
      }

      static inline bool
      Decode(const BufferPtr& buff, BlockMessage& result, const codec::DecoderHints& hints=codec::kDefaultDecoderHints){
        Decoder decoder(hints);
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