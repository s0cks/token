#ifndef TOKEN_RPC_MESSAGE_TRANSACTION_H
#define TOKEN_RPC_MESSAGE_TRANSACTION_H

#include "network/rpc_message_object.h"

namespace token{
  namespace rpc{
    class TransactionMessage : public ObjectMessage<Transaction>{
     public:
      class Encoder : public ObjectMessageEncoder<TransactionMessage>{
       public:
        Encoder(const TransactionMessage& value, const codec::EncoderFlags& flags=codec::kDefaultEncoderFlags):
          ObjectMessageEncoder<TransactionMessage>(value, flags){}
        Encoder(const Encoder& other) = default;
        ~Encoder() override = default;
        int64_t GetBufferSize() const override;
        bool Encode(const BufferPtr& buff) const override;
        Encoder& operator=(const Encoder& other) = delete; //TODO: don't delete this copy-assignment operator
      };

      class Decoder : public ObjectMessageDecoder<TransactionMessage>{
       public:
        Decoder(const codec::DecoderHints& hints=codec::kDefaultDecoderHints):
          ObjectMessageDecoder<TransactionMessage>(hints){}
        Decoder(const Decoder& other) = default;
        ~Decoder() override = default;
        bool Decode(const BufferPtr& buff, TransactionMessage& result) const override;
        Decoder& operator=(const Decoder& other) = default;
      };
     public:
      TransactionMessage() = default;
      TransactionMessage(const TransactionPtr& val):
        ObjectMessage<Transaction>(val){}
      TransactionMessage(const TransactionMessage& other) = default;
      ~TransactionMessage() override = default;

      Type type() const override{
        return Type::kTransactionMessage;
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
      TransactionMessage& operator=(const TransactionMessage& other) = default;

      static inline TransactionMessagePtr
      NewInstance(const TransactionPtr& value){
        return std::make_shared<TransactionMessage>(value);
      }

      static inline bool
      Decode(const BufferPtr& buff, TransactionMessage& result, const codec::DecoderHints& hints=codec::kDefaultDecoderHints){
        Decoder decoder(hints);
        return decoder.Decode(buff, result);
      }

      static inline TransactionMessagePtr
      DecodeNew(const BufferPtr& buff, const codec::DecoderHints& hints=codec::kDefaultDecoderHints){
        TransactionMessage instance;
        if(!Decode(buff, instance, hints)){
          DLOG(ERROR) << "cannot decode TransactionMessage.";
          return nullptr;
        }
        return std::make_shared<TransactionMessage>(instance);
      }
    };
  }
}

#endif//TOKEN_RPC_MESSAGE_TRANSACTION_H