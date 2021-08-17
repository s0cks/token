#ifndef TOKEN_TRANSACTION_INPUT_H
#define TOKEN_TRANSACTION_INPUT_H

#include <vector>

#include "input.pb.h"

#include "object.h"
#include "type/user.h"
#include "codec/codec.h"
#include "transaction_reference.h"

namespace token{

  typedef internal::proto::Input RawInput;
  class Input : public Object{
   private:
    RawInput raw_;
    Hash hash_;
   public:
    Input():
      Object(),
      raw_(){}
    explicit Input(const RawInput& raw):
      Input(){
      raw_.CopyFrom(raw);
    }
    explicit Input(const internal::BufferPtr& data):
      Input(){
      if(!raw_.ParseFromArray(data->data(), static_cast<int>(data->length())))
        DLOG(FATAL) << "cannot parse Input from buffer.";
    }
    Input(const Hash& hash, const Hash& tx, const uint64_t& index):
      Object(),
      raw_(),
      hash_(hash){
      raw_.set_hash(hash.HexString());
      raw_.set_transaction(tx.HexString());
      raw_.set_index(index);
    }
    Input(const Input& other):
      Input(){
      raw_.CopyFrom(other.raw_);
    }
    ~Input() override = default;

    Type type() const override{
      return Type::kInput;
    }

    Hash utxo_hash() const{
      return Hash::FromHexString(raw_.hash());
    }

    Hash transaction_hash() const{
      return Hash(raw_.transaction());
    }

    uint64_t index() const{
      return raw_.index();
    }

    TransactionReference reference() const{
      return TransactionReference(transaction_hash(), index());
    }

    internal::BufferPtr ToBuffer() const;

    /**
     * Returns the description of this object.
     *
     * @see Object::ToString()
     * @return The ToString() description of this object
     */
    std::string ToString() const override;

    Input& operator=(const Input& other){
      if(&other == this)
        return *this;
      raw_.CopyFrom(other.raw_);
      return *this;
    }

    friend bool operator==(const Input& a, const Input& b){
      return a.utxo_hash() == b.utxo_hash();
    }

    friend bool operator!=(const Input& a, const Input& b){
      return a.utxo_hash() != b.utxo_hash();
    }

    friend bool operator<(const Input& a, const Input& b){
      return a.utxo_hash() < b.utxo_hash();
    }

    friend bool operator>(const Input& a, const Input& b){
      return a.utxo_hash() > b.utxo_hash();
    }

    friend std::ostream& operator<<(std::ostream& stream, const Input& input){
      return stream << input.ToString();
    }
  };

namespace json{
    static inline bool
    Write(Writer& writer, const Input& val){
      JSON_START_OBJECT(writer);
      {
//TODO:
//        if(!json::SetField(writer, "transaction", val.GetReference()))
//          return false;
//        if(!json::SetField(writer, "user", val.GetUser()))
//          return false;
      }
      JSON_END_OBJECT(writer);
      return true;
    }

    static inline bool
    SetField(Writer& writer, const char* name, const Input& val){
      JSON_KEY(writer, name);
      return Write(writer, val);
    }
  }
}

#endif//TOKEN_TRANSACTION_INPUT_H