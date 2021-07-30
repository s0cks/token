#ifndef TOKEN_TRANSACTION_OUTPUT_H
#define TOKEN_TRANSACTION_OUTPUT_H

#include <vector>

#include "object.h"
#include "type/user.h"
#include "type/product.h"

#include "codec/codec.h"

namespace token{
  class Output : public Object{
    friend class Transaction;
  public:
    class Encoder : public codec::TypeEncoder<Output>{
    private:
      User::Encoder encode_user_;
      Product::Encoder encode_product_;
    public:
      Encoder(const Output* value, const codec::EncoderFlags& flags);
      Encoder(const Encoder& other) = default;
      ~Encoder() override = default;
      int64_t GetBufferSize() const override;
      bool Encode(const BufferPtr& buff) const override;
      Encoder& operator=(const Encoder& other) = default;
    };

    class Decoder : public codec::TypeDecoder<Output>{
    private:
      User::Decoder decode_user_;
      Product::Decoder decode_product_;
    public:
      explicit Decoder(const codec::DecoderHints& hints);
      ~Decoder() override = default;
      Output* Decode(const BufferPtr& data) const override;
    };
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

    static inline Output*
    Decode(const BufferPtr& data, const codec::DecoderHints& hints=codec::kDefaultDecoderHints){
      Decoder decoder(hints);
      return decoder.Decode(data);
    }

    static inline std::shared_ptr<Output>
    DecodeNew(const BufferPtr& data, const codec::DecoderHints& hints=codec::kDefaultDecoderHints){
      return std::shared_ptr<Output>(Decode(data, hints));
    }
  };

  typedef std::vector<Output> OutputList;

  namespace codec{
    class OutputListEncoder : public codec::ListEncoder<Output>{
     public:
      OutputListEncoder(const OutputList& items, const codec::EncoderFlags& flags):
        codec::ListEncoder<Output>(items, flags){}
      OutputListEncoder(const OutputListEncoder& other) = default;
      ~OutputListEncoder() override = default;
      OutputListEncoder& operator=(const OutputListEncoder& other) = default;
    };

    class OutputListDecoder : public codec::ArrayDecoder<Output, Output::Decoder>{
     public:
      explicit OutputListDecoder(const codec::DecoderHints& hints):
        codec::ArrayDecoder<Output, Output::Decoder>(hints){}
      ~OutputListDecoder() override = default;
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