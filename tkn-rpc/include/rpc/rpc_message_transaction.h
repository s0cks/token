#ifndef TOKEN_RPC_MESSAGE_TRANSACTION_H
#define TOKEN_RPC_MESSAGE_TRANSACTION_H

#include "../../../Sources/token/transaction_unsigned.h"
#include "rpc/rpc_message_object.h"

namespace token{
  namespace rpc{
    class TransactionMessage;
  }

  namespace codec{
    class TransactionMessageEncoder : public ObjectMessageEncoder<rpc::TransactionMessage, UnsignedTransactionEncoder>{
    public:
      TransactionMessageEncoder(const rpc::TransactionMessage& value, const codec::EncoderFlags& flags):
        ObjectMessageEncoder<rpc::TransactionMessage, UnsignedTransactionEncoder>(value, flags){}
      TransactionMessageEncoder(const TransactionMessageEncoder& other) = default;
      ~TransactionMessageEncoder() override = default;
      TransactionMessageEncoder& operator=(const TransactionMessageEncoder& other) = delete; //TODO: don't delete this copy-assignment operator
    };

  class TransactionMessageDecoder : public ObjectMessageDecoder<rpc::TransactionMessage, UnsignedTransactionDecoder>{
    public:
      explicit TransactionMessageDecoder(const codec::DecoderHints& hints):
        ObjectMessageDecoder<rpc::TransactionMessage, UnsignedTransactionDecoder>(hints){}
      TransactionMessageDecoder(const TransactionMessageDecoder& other) = default;
      ~TransactionMessageDecoder() override = default;
      bool Decode(const BufferPtr& buff, rpc::TransactionMessage& result) const override;
      TransactionMessageDecoder& operator=(const TransactionMessageDecoder& other) = default;
    };
  }

  namespace rpc{
    class TransactionMessage : public ObjectMessage<UnsignedTransaction>{
     public:
      TransactionMessage() = default;
      explicit TransactionMessage(const UnsignedTransactionPtr& val):
        ObjectMessage<UnsignedTransaction>(val){}
      TransactionMessage(const TransactionMessage& other) = default;
      ~TransactionMessage() override = default;

      Type type() const override{
        return Type::kTransactionMessage;
      }

      int64_t GetBufferSize() const override{
        codec::TransactionMessageEncoder encoder((*this), GetDefaultMessageEncoderFlags());
        return encoder.GetBufferSize();
      }

      bool Write(const BufferPtr& buff) const override{
        codec::TransactionMessageEncoder encoder((*this), GetDefaultMessageEncoderFlags());
        return encoder.Encode(buff);
      }

      std::string ToString() const override{
        std::stringstream ss;
        ss << "TransactionMessage(";
        ss << "value=" << value()->ToString();
        ss << ")";
        return ss.str();
      }

      TransactionMessage& operator=(const TransactionMessage& other) = default;

      static inline TransactionMessagePtr
      NewInstance(const UnsignedTransactionPtr& value){
        return std::make_shared<TransactionMessage>(value);
      }

      static inline bool
      Decode(const BufferPtr& buff, TransactionMessage& result, const codec::DecoderHints& hints=codec::kDefaultDecoderHints){
        codec::TransactionMessageDecoder decoder(hints);
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