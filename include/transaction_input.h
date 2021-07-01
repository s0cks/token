#ifndef TOKEN_TRANSACTION_INPUT_H
#define TOKEN_TRANSACTION_INPUT_H

#include <vector>

#include "user.h"
#include "codec.h"
#include "encoder.h"
#include "decoder.h"
#include "binary_object.h"
#include "transaction_reference.h"

namespace token{
  class Input : public BinaryObject{
    friend class Transaction;
   public:
    class Encoder : public codec::EncoderBase<Input>{
     public:
      Encoder(const Input& value, const codec::EncoderFlags& flags=codec::kDefaultEncoderFlags):
        codec::EncoderBase<Input>(value, flags){}
      Encoder(const Encoder& other) = default;
      ~Encoder() override = default;

      int64_t GetBufferSize() const override;
      bool Encode(const BufferPtr& buff) const override;

      Encoder& operator=(const Encoder& other) = default;
    };

    class Decoder : public codec::DecoderBase<Input>{
     public:
      Decoder(const codec::DecoderHints& hints=codec::kDefaultDecoderHints):
        codec::DecoderBase<Input>(hints){}
      Decoder(const Decoder& other) = default;
      ~Decoder() override = default;

      bool Decode(const BufferPtr& buff, Input& result) const override;

      Decoder& operator=(const Decoder& other) = default;
    };
   private:
    TransactionReference reference_;
    User user_;//TODO: remove field
   public:
    Input():
      reference_(),
      user_(){}
    Input(const TransactionReference& ref, const User& user):
      BinaryObject(),
      reference_(ref),
      user_(user){}
    Input(const Hash& tx, int64_t index, const User& user):
      BinaryObject(),
      reference_(tx, index),
      user_(user){}
    Input(const Hash& tx, int64_t index, const std::string& user):
      BinaryObject(),
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

    BufferPtr ToBuffer() const override;

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
      Input::Decoder decoder(hints);
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
}

#endif//TOKEN_TRANSACTION_INPUT_H