#ifndef TOKEN_TRANSACTION_INPUT_H
#define TOKEN_TRANSACTION_INPUT_H

#include <vector>
#include "object.h"
#include "type/user.h"
#include "codec/codec.h"
#include "transaction_reference.h"

namespace token{
  namespace codec{
    class InputEncoder : public codec::EncoderBase<Input>{
     protected:
      TransactionReferenceEncoder encode_txref_;
      UserEncoder encode_user_;
     public:
      explicit InputEncoder(const Input& value, const codec::EncoderFlags& flags=codec::kDefaultEncoderFlags);
      InputEncoder(const InputEncoder& other) = default;
      ~InputEncoder() override = default;

      int64_t GetBufferSize() const override;
      bool Encode(const BufferPtr& buff) const override;
      InputEncoder& operator=(const InputEncoder& other) = default;
    };

    class InputDecoder : public codec::DecoderBase<Input>{
     protected:
      TransactionReferenceDecoder decode_txref_;
      UserDecoder decode_user_;
     public:
      explicit InputDecoder(const codec::DecoderHints& hints=codec::kDefaultDecoderHints):
        codec::DecoderBase<Input>(hints),
        decode_txref_(hints),
        decode_user_(hints){}
      InputDecoder(const InputDecoder& other) = default;
      ~InputDecoder() override = default;
      bool Decode(const BufferPtr& buff, Input& result) const override;
      InputDecoder& operator=(const InputDecoder& other) = default;
    };
  }

  class Input : public Object{
    friend class Transaction;
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

    static inline bool
    Decode(const BufferPtr& buff, Input& result, const codec::DecoderHints& hints=codec::kDefaultDecoderHints){
      codec::InputDecoder decoder(hints);
      return decoder.Decode(buff, result);
    }

    static inline std::shared_ptr<Input>
    DecodeNew(const BufferPtr& buff, const codec::DecoderHints& hints=codec::kDefaultDecoderHints){
      Input result;
      if(!Decode(buff, result, hints)){
        DLOG(WARNING) << "couldn't decode Input.";
        return nullptr;
      }

      return std::make_shared<Input>(result);
    }
  };

typedef std::vector<Input> InputList;

namespace codec{
 class InputListEncoder : public codec::ListEncoder<Input, InputEncoder>{
   public:
    InputListEncoder(const InputList& items, const codec::EncoderFlags& flags):
        codec::ListEncoder<Input, InputEncoder>(Type::kInputList, items, flags){}
    InputListEncoder(const InputListEncoder& other) = default;
    ~InputListEncoder() override = default;
    InputListEncoder& operator=(const InputListEncoder& other) = default;
  };

  class InputListDecoder : public codec::ListDecoder<Input, InputDecoder>{
   public:
    explicit InputListDecoder(const codec::DecoderHints &hints):
      codec::ListDecoder<Input, InputDecoder>(hints){}
    InputListDecoder(const InputListDecoder &other) = default;
    ~InputListDecoder() override = default;
    InputListDecoder& operator=(const InputListDecoder& other) = default;
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