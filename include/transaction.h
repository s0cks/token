#ifndef TOKEN_TRANSACTION_H
#define TOKEN_TRANSACTION_H

#include <set>
#include <utility>

#include "codec.h"
#include "encoder.h"
#include "decoder.h"
#include "timestamp.h"
#include "binary_object.h"
#include "transaction_input.h"
#include "transaction_output.h"

namespace token{
  namespace internal{
    class TransactionBase : public BinaryObject{
     protected:
      template<class T>
      class TransactionEncoder : public codec::EncoderBase<T>{
       private:
        typedef codec::EncoderBase<T> BaseType;

        void EncodeInputs(const BufferPtr& buff) const{
          const InputList& items = BaseType::value().inputs();
          InputListEncoder encoder(items, BaseType::flags());
          if(!encoder.Encode(buff))
            LOG(FATAL) << "couldn't serialize InputList: " << items.size();//TODO: better error handling
        }

        void EncodeOutputs(const BufferPtr& buff) const{
          const OutputList& items = BaseType::value().outputs();
          OutputListEncoder encoder(items, BaseType::flags());
          if(!encoder.Encode(buff))
            LOG(FATAL) << "couldn't serialize OutputList: " << items.size();//TODO: better error-handling
        }
       protected:
        explicit TransactionEncoder(const T& value, const codec::EncoderFlags& flags=codec::kDefaultEncoderFlags):
          BaseType(value, flags){}
       public:
        TransactionEncoder(const TransactionEncoder& other) = default;
        ~TransactionEncoder() override = default;

        int64_t GetBufferSize() const override{
          int64_t size = BaseType::GetBufferSize();
          size += sizeof(RawTimestamp);
          size += BaseType::template GetBufferSize<Input, Input::Encoder>(BaseType::value().inputs());
          size += BaseType::template GetBufferSize<Output, Output::Encoder>(BaseType::value().outputs());
          return size;
        }

        bool Encode(const BufferPtr& buff) const override{
          //TODO: encode type
          //TODO: encode version
          const Timestamp& timestamp = BaseType::value().timestamp();
          if(!buff->PutTimestamp(timestamp)){
            LOG(FATAL) << "couldn't encode timestamp: " << FormatTimestampReadable(timestamp);
            return false;
          }

          EncodeInputs(buff);
          EncodeOutputs(buff);
          return true;
        }

        TransactionEncoder& operator=(const TransactionEncoder& other) = default;
      };

      template<class T>
      class TransactionDecoder : public codec::DecoderBase<T>{
       private:
        typedef codec::DecoderBase<T> BaseType;
       protected:
        explicit TransactionDecoder(const codec::DecoderHints& hints=codec::kDefaultDecoderHints):
          BaseType(hints){}
       public:
        TransactionDecoder(const TransactionDecoder& other) = default;
        ~TransactionDecoder() override = default;

        bool Decode(const BufferPtr& buff, T& result) const override{
          if(BaseType::ShouldExpectType()){}
          if(BaseType::ShouldExpectVersion()){}

          return true;
        }

        TransactionDecoder& operator=(const TransactionDecoder& other) = default;
      };

      Timestamp timestamp_;
      InputList inputs_;
      OutputList outputs_;

      TransactionBase():
        BinaryObject(),
        timestamp_(),
        inputs_(),
        outputs_(){}
      TransactionBase(const Timestamp& timestamp, InputList inputs, OutputList outputs):
        BinaryObject(),
        timestamp_(timestamp),
        inputs_(std::move(inputs)),
        outputs_(std::move(outputs)){}
     public:
      TransactionBase(const TransactionBase& other) = default;
      ~TransactionBase() override = default;

      Timestamp& timestamp(){
        return timestamp_;
      }

      Timestamp timestamp() const{
        return timestamp_;
      }

      InputList& inputs(){
        return inputs_;
      }

      InputList inputs() const{
        return inputs_;
      }

      InputList::iterator inputs_begin(){
        return inputs_.begin();
      }

      InputList::const_iterator inputs_begin() const{
        return inputs_.begin();
      }

      InputList::iterator inputs_end(){
        return inputs_.end();
      }

      InputList::const_iterator inputs_end() const{
        return inputs_.end();
      }

      OutputList& outputs(){
        return outputs_;
      }

      OutputList outputs() const{
        return outputs_;
      }

      OutputList::iterator outputs_begin(){
        return outputs_.begin();
      }

      OutputList::const_iterator outputs_begin() const{
        return outputs_.begin();
      }

      OutputList::iterator outputs_end(){
        return outputs_.end();
      }

      OutputList::const_iterator outputs_end() const{
        return outputs_.end();
      }

      virtual bool IsSigned() const{
        return false;
      }

      TransactionBase& operator=(const TransactionBase& other) = default;
    };
  }
}

#endif //TOKEN_TRANSACTION_H