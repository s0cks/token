#ifndef TOKEN_RPC_MESSAGE_BLOCK_H
#define TOKEN_RPC_MESSAGE_BLOCK_H

#include "block.h"
#include "rpc/rpc_message_object.h"

namespace token{
  namespace rpc{
    class BlockMessage;
  }

  namespace codec{
    class BlockMessageEncoder : public ObjectMessageEncoder<rpc::BlockMessage, BlockEncoder>{
    public:
      BlockMessageEncoder(const rpc::BlockMessage& value, const codec::EncoderFlags& flags):
        ObjectMessageEncoder<rpc::BlockMessage, BlockEncoder>(value, flags){}
      BlockMessageEncoder(const BlockMessageEncoder& other) = default;
      ~BlockMessageEncoder() override = default;
      BlockMessageEncoder& operator=(const BlockMessageEncoder& other) = delete;//TODO: don't delete this copy assignment operator
    };

    class BlockMessageDecoder : public ObjectMessageDecoder<rpc::BlockMessage, BlockDecoder>{
    public:
      explicit BlockMessageDecoder(const DecoderHints& hints):
        ObjectMessageDecoder<rpc::BlockMessage, BlockDecoder>(hints){}
      BlockMessageDecoder(const BlockMessageDecoder& rhs) = default;
      ~BlockMessageDecoder() override = default;
      bool Decode(const BufferPtr& buff, rpc::BlockMessage& result) const override;
      BlockMessageDecoder& operator=(const BlockMessageDecoder& rhs) = default;
    };
  }

  namespace rpc{
    class BlockMessage : public ObjectMessage<Block>{
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
        codec::BlockMessageEncoder encoder((*this), GetDefaultMessageEncoderFlags());
        return encoder.GetBufferSize();
      }

      bool Write(const BufferPtr& buff) const override{
        codec::BlockMessageEncoder encoder((*this), GetDefaultMessageEncoderFlags());
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
        codec::BlockMessageDecoder decoder(hints);
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