#ifndef TOKEN_TRANSACTION_OUTPUT_H
#define TOKEN_TRANSACTION_OUTPUT_H

#include <vector>

#include "user.h"
#include "codec.h"
#include "encoder.h"
#include "decoder.h"
#include "product.h"
#include "binary_object.h"

namespace token{
  class Output : public BinaryObject{
    friend class Transaction;
   public:
    class Encoder : public codec::EncoderBase<Output>{
     private:
      User::Encoder user_encoder_;
      Product::Encoder product_encoder_;
     public:
      Encoder(const Output& value, const codec::EncoderFlags& flags):
        codec::EncoderBase<Output>(value, flags),
        user_encoder_(value.GetUser(), flags),
        product_encoder_(value.GetProduct(), flags){}
      Encoder(const Encoder& other) = default;
      ~Encoder() override = default;

      int64_t GetBufferSize() const override{
        int64_t size = codec::EncoderBase<Output>::GetBufferSize();
        size += user_encoder_.GetBufferSize();
        size += product_encoder_.GetBufferSize();
        return size;
      }

      bool Encode(const BufferPtr& buff) const override{
        if(!BaseType::Encode(buff))
          return false;
        if(!user_encoder_.Encode(buff))
          return false;
        if(!product_encoder_.Encode(buff))
          return false;
        return true;
      }

      Encoder& operator=(const Encoder& other) = default;
    };

    class Decoder : public codec::DecoderBase<Output>{
     private:
      User::Decoder user_decoder_;
      Product::Decoder product_decoder_;
     public:
      explicit Decoder(const codec::DecoderHints& hints=codec::kDefaultDecoderHints):
        codec::DecoderBase<Output>(hints),
        user_decoder_(hints),
        product_decoder_(hints){}
      Decoder(const Decoder& other) = default;
      ~Decoder() override = default;

      bool Decode(const BufferPtr& buff, Output& result) const override{
        //TODO: decode type
        //TODO: decode version

        User user;
        if(!user_decoder_.Decode(buff, user))
          return false;

        Product product;
        if(!product_decoder_.Decode(buff, product))
          return false;

        result = Output(user, product);
        return true;
      }

      Decoder& operator=(const Decoder& other) = default;
    };
   private:
    User user_;
    Product product_;
   public:
    Output():
      BinaryObject(),
      user_(),
      product_(){}
    Output(const User& user, const Product& product):
      BinaryObject(),
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

    BufferPtr ToBuffer() const override;

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
      Decoder decoder(hints);
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

  class OutputListEncoder : public codec::ListEncoder<Output, Output::Encoder>{
  private:
   typedef codec::ListEncoder<Output, Output::Encoder> BaseType;
  public:
   OutputListEncoder(const OutputList& items, const codec::EncoderFlags& flags):
    BaseType(Type::kOutputList, items, flags){}
    OutputListEncoder(const OutputListEncoder& other) = default;
   ~OutputListEncoder() override = default;
   OutputListEncoder& operator=(const OutputListEncoder& other) = default;
  };

  class OutputListDecoder : public codec::ListDecoder<Output, Output::Decoder>{
  private:
   typedef codec::ListDecoder<Output, Output::Decoder> BaseType;
  public:
   explicit OutputListDecoder(const codec::DecoderHints& hints):
    BaseType(hints){}
  OutputListDecoder(const OutputListDecoder& other) = default;
   ~OutputListDecoder() override = default;
   OutputListDecoder& operator=(const OutputListDecoder& other) = default;
  };

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