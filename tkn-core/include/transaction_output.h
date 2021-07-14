#ifndef TOKEN_TRANSACTION_OUTPUT_H
#define TOKEN_TRANSACTION_OUTPUT_H

#include <vector>

#include "object.h"
#include "type/user.h"
#include "type/product.h"

#include "codec/codec.h"

namespace token{
  namespace codec{
    class OutputEncoder : public codec::EncoderBase<Output>{
     private:
      UserEncoder encode_user_;
      ProductEncoder encode_product_;
     public:
      OutputEncoder(const Output& value, const codec::EncoderFlags& flags);
      OutputEncoder(const OutputEncoder& other) = default;
      ~OutputEncoder() override = default;
      int64_t GetBufferSize() const override;
      bool Encode(const BufferPtr& buff) const override;
      OutputEncoder& operator=(const OutputEncoder& other) = default;
    };

    class OutputDecoder : public codec::DecoderBase<Output>{
     private:
      UserDecoder decode_user_;
      ProductDecoder decode_product_;
     public:
      explicit OutputDecoder(const codec::DecoderHints& hints=codec::kDefaultDecoderHints);
      OutputDecoder(const OutputDecoder& other) = default;
      ~OutputDecoder() override = default;
      bool Decode(const BufferPtr& buff, Output& result) const override;
      OutputDecoder& operator=(const OutputDecoder& other) = default;
    };
  }

  class Output : public Object{
    friend class Transaction;
   private:
    User user_;
    Product product_;
   public:
    Output():
      Object(),
      user_(),
      product_(){}
    Output(const User& user, const Product& product):
      Object(),
      user_(user),
      product_(product){}
    Output(const std::string& user, const Product& product):
      Output(User(user), product){}
    Output(const std::string& user, const std::string& product):
      Output(User(user), Product(product)){}
    Output(const char* user, const char* product):
      Output(User(user), Product(product)){}
    Output(const Output& other) = default;
    ~Output() override = default;

    Type type() const override{
      return Type::kOutput;
    }

    /**
     * Returns the User for this Output.
     *
     * @see User
     * @return The User for this Output
     */
    User GetUser() const{
      return user_;
    }

    /**
     * Returns the Product for this Output.
     *
     * @see Product
     * @return The Product for this Output
     */
    Product GetProduct() const{
      return product_;
    }

    /**
     * Returns the description of this object.
     *
     * @see Object::ToString()
     * @return The ToString() description of this object
     */
    std::string ToString() const override;

    Output& operator=(const Output& other) = default;

    friend bool operator==(const Output& a, const Output& b){
      return a.user_ == b.user_
          && a.product_ == b.product_;
    }

    friend bool operator!=(const Output& a, const Output& b){
      return !operator==(a, b);
    }

    friend bool operator<(const Output& a, const Output& b){
      if(a.user_ == b.user_){
        return a.product_ < b.product_;
      }
      return a.user_ < b.user_;
    }

    friend bool operator>(const Output& a, const Output& b){
      if(a.user_ == b.user_)
        return a.product_ > b.product_;
      return a.user_ > b.user_;
    }

    friend std::ostream& operator<<(std::ostream& stream, const Output& output){
      return stream << output.ToString();
    }

    static inline bool
    Decode(const BufferPtr& buff, Output& result, const codec::DecoderHints& hints=codec::kDefaultDecoderHints){
      codec::OutputDecoder decoder(hints);
      return decoder.Decode(buff, result);
    }

    static inline std::shared_ptr<Output>
    DecodeNew(const BufferPtr& buff, const codec::DecoderHints& hints=codec::kDefaultDecoderHints){
      Output result;
      if(!Decode(buff, result, hints)){
        DLOG(WARNING) << "cannot decode Output.";
        return nullptr;
      }

      return std::make_shared<Output>(result);
    }
  };

  typedef std::vector<Output> OutputList;

  namespace codec{
    class OutputListEncoder : public codec::ListEncoder<Output, OutputEncoder>{
     public:
      OutputListEncoder(const OutputList& items, const codec::EncoderFlags& flags):
        codec::ListEncoder<Output, OutputEncoder>(Type::kOutputList, items, flags){}
      OutputListEncoder(const OutputListEncoder& other) = default;
      ~OutputListEncoder() override = default;
      OutputListEncoder& operator=(const OutputListEncoder& other) = default;
    };

    class OutputListDecoder : public codec::ListDecoder<Output, OutputDecoder>{
     public:
      explicit OutputListDecoder(const codec::DecoderHints& hints):
        codec::ListDecoder<Output, OutputDecoder>(hints){}
      OutputListDecoder(const OutputListDecoder& other) = default;
      ~OutputListDecoder() override = default;
      OutputListDecoder& operator=(const OutputListDecoder& other) = default;
    };
  }

  namespace json{
    static inline bool
    Write(Writer& writer, const Output& val){
      JSON_START_OBJECT(writer);
      {
        if(!json::SetField(writer, "user", val.GetUser()))
          return false;
        if(!json::SetField(writer, "product", val.GetProduct()))
          return false;
      }
      JSON_END_OBJECT(writer);
      return true;
    }

    static inline bool
    SetField(Writer& writer, const char* name, const Output& val){
      JSON_KEY(writer, name);
      return Write(writer, val);
    }

    static inline bool
    Write(Writer& writer, const OutputList& val){
      JSON_START_ARRAY(writer);
      {
        for(auto& it : val)
          if(!Write(writer, it))
            return false;
      }
      JSON_END_ARRAY(writer);
      return true;
    }

    static inline bool
    SetField(Writer& writer, const char* name, const OutputList& val){
      JSON_KEY(writer, name);
      return Write(writer, val);
    }
  }
}

#endif//TOKEN_TRANSACTION_OUTPUT_H