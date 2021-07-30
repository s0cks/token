#ifndef TOKEN_TRANSACTION_H
#define TOKEN_TRANSACTION_H

#include <set>
#include <utility>

#include "object.h"
#include "timestamp.h"
#include "transaction_input.h"
#include "transaction_output.h"

namespace token{
  namespace internal{
    class TransactionBase : public BinaryObject{
     protected:
      template<class T>
      class TransactionEncoder : public codec::TypeEncoder<T>{
      private:
        codec::InputListEncoder encode_inputs_;
        codec::OutputListEncoder encode_outputs_;
      protected:
        explicit TransactionEncoder(const T* value, const codec::EncoderFlags& flags=codec::kDefaultEncoderFlags):
            codec::TypeEncoder<T>(value, flags),
            encode_inputs_(value->inputs_, flags),
            encode_outputs_(value->outputs_, flags){}
      public:
        TransactionEncoder(const TransactionEncoder& other) = default;
        ~TransactionEncoder() override = default;

        int64_t GetBufferSize() const override{
          auto size = codec::TypeEncoder<T>::GetBufferSize();
          size += sizeof(RawTimestamp);
          size += encode_inputs_.GetBufferSize();
          size += encode_outputs_.GetBufferSize();
          return size;
        }

        bool Encode(const BufferPtr& buff) const override{
          if(!codec::TypeEncoder<T>::Encode(buff))
            return false;

          const auto& timestamp = codec::TypeEncoder<T>::value()->timestamp();
          if(!buff->PutTimestamp(timestamp)){
            LOG(FATAL) << "cannot encode timestamp to buffer.";
            return false;
          }

          if(!encode_inputs_.Encode(buff)){
            LOG(FATAL) << "cannot encode input list to buffer.";
            return false;
          }

          if(!encode_outputs_.Encode(buff)){
            LOG(FATAL) << "cannot encode output list to buffer.";
            return false;
          }
          return true;
        }

        TransactionEncoder& operator=(const TransactionEncoder& other) = default;
      };

      template<class T>
      class TransactionDecoder : public codec::TypeDecoder<T>{
      protected:
        codec::InputListDecoder decode_inputs_;
        codec::OutputListDecoder decode_outputs_;

        explicit TransactionDecoder(const codec::DecoderHints& hints=codec::kDefaultDecoderHints):
            codec::TypeDecoder<T>(hints),
            decode_inputs_(hints),
            decode_outputs_(hints){}

        bool DecodeTransactionData(const BufferPtr& buff, Timestamp& timestamp, InputList& inputs, OutputList& outputs) const{
          timestamp = buff->GetTimestamp();
          DLOG(INFO) << "decoded transaction timestamp: " << FormatTimestampReadable(timestamp);

          if(!decode_inputs_.Decode(buff, inputs)){
            LOG(FATAL) << "cannot decode InputList from buffer.";
            return false;
          }

          if(!decode_outputs_.Decode(buff, outputs)){
            LOG(FATAL) << "cannot decode OutputList from buffer.";
            return false;
          }
          return true;
        }
      public:
        TransactionDecoder(const TransactionDecoder& other) = default;
        ~TransactionDecoder() override = default;
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