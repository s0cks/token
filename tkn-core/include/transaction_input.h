#ifndef TOKEN_TRANSACTION_INPUT_H
#define TOKEN_TRANSACTION_INPUT_H

#include <vector>
#include "object.h"
#include "type/user.h"
#include "codec/codec.h"
#include "transaction_reference.h"

namespace token{
  typedef std::shared_ptr<Input> InputPtr;

  class Input : public Object{
    friend class Transaction;
  public:
    class Encoder : public codec::TypeEncoder<Input>{
    protected:
      TransactionReference::Encoder encode_txref_;
      User::Encoder encode_user_;
    public:
      explicit Encoder(const Input* value, const codec::EncoderFlags& flags=codec::kDefaultEncoderFlags);
      Encoder(const Encoder& other) = default;
      ~Encoder() override = default;

      int64_t GetBufferSize() const override;
      bool Encode(const BufferPtr& buff) const override;
      Encoder& operator=(const Encoder& other) = default;
    };

    class Decoder : public codec::TypeDecoder<Input>{
    protected:
      TransactionReference::Decoder decode_txref_;
      User::Decoder decode_user_;
    public:
      explicit Decoder(const codec::DecoderHints& hints):
          codec::TypeDecoder<Input>(hints),
          decode_txref_(hints),
          decode_user_(hints){}
      ~Decoder() override = default;
      Input* Decode(const BufferPtr& data) const override;
    };
   private:
    TransactionReference reference_;
    User user_;//TODO: remove field
   public:
    Input():
      reference_(),
      user_(){}
    Input(const TransactionReference& ref, const User& user):
      Object(),
      reference_(ref),
      user_(user){}
    Input(const Hash& tx, int64_t index, const User& user):
      Object(),
      reference_(tx, index),
      user_(user){}
    Input(const Hash& tx, int64_t index, const std::string& user):
      Object(),
      reference_(tx, index),
      user_(user){}
    Input(const Input& other) = default;
    ~Input() override = default;

    Type type() const override{
      return Type::kInput;
    }

    /**
     * Returns the TransactionReference for this input.
     *
     * @see TransactionReference
     * @return The TransactionReference
     */
    TransactionReference& GetReference(){
      return reference_;
    }

    /**
     * Returns a copy of the TransactionReference for this input.
     *
     * @see TransactionReference
     * @return A copy of the TransactionReference
     */
    TransactionReference GetReference() const{
      return reference_;
    }

    /**
     * Returns the User for this input.
     *
     * @see User
     * @deprecated
     * @return The User for this input
     */
    User& GetUser(){
      return user_;
    }

    /**
     * Returns a copy of the User for this input.
     *
     * @see User
     * @deprecated
     * @return A copy of the User for this input
     */
    User GetUser() const{
      return user_;
    }

    /**
     * Returns the description of this object.
     *
     * @see Object::ToString()
     * @return The ToString() description of this object
     */
    std::string ToString() const override;

    Input& operator=(const Input& other) = default;

    friend bool operator==(const Input& a, const Input& b){
      return a.reference_ == b.reference_
          && a.user_ == b.user_;
    }

    friend bool operator!=(const Input& a, const Input& b){
      return !operator==(a, b);
    }

    friend bool operator<(const Input& a, const Input& b){
      return a.reference_ < b.reference_;
    }

    friend bool operator>(const Input& a, const Input& b){
      return a.reference_ > b.reference_;
    }

    friend std::ostream& operator<<(std::ostream& stream, const Input& input){
      return stream << input.ToString();
    }

    static inline InputPtr
    Decode(const BufferPtr& data, const codec::DecoderHints& hints=codec::kDefaultDecoderHints){
      Decoder decoder(hints);

      Input* value = nullptr;
      if(!(value = decoder.Decode(data))){
        DLOG(FATAL) << "cannot decode Input from buffer.";
        return nullptr;
      }
      return std::shared_ptr<Input>(value);
    }
  };

typedef std::vector<std::shared_ptr<Input>> InputList;

namespace codec{
class InputListEncoder : public codec::ListEncoder<Input>{
   public:
    InputListEncoder(const InputList& items, const codec::EncoderFlags& flags):
      codec::ListEncoder<Input>(items, flags){}
    ~InputListEncoder() override = default;
  };

class InputListDecoder : public codec::ArrayDecoder<Input, Input::Decoder>{
   public:
    explicit InputListDecoder(const codec::DecoderHints &hints):
      codec::ArrayDecoder<Input, Input::Decoder>(hints){}
    ~InputListDecoder() override = default;
  };
}

namespace json{
    static inline bool
    Write(Writer& writer, const Input& val){
      JSON_START_OBJECT(writer);
      {
        if(!json::SetField(writer, "transaction", val.GetReference()))
          return false;
        if(!json::SetField(writer, "user", val.GetUser()))
          return false;
      }
      JSON_END_OBJECT(writer);
      return true;
    }

    static inline bool
    SetField(Writer& writer, const char* name, const Input& val){
      JSON_KEY(writer, name);
      return Write(writer, val);
    }

    static inline bool
    Write(Writer& writer, const InputList& val){
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
    SetField(Writer& writer, const char* name, const InputList& val){
      JSON_KEY(writer, name);
      return Write(writer, val);
    }
  }
}

#endif//TOKEN_TRANSACTION_INPUT_H