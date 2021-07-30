#ifndef TOKEN_TRANSACTION_INPUT_H
#define TOKEN_TRANSACTION_INPUT_H

#include <vector>

#include "input.pb.h"

#include "object.h"
#include "type/user.h"
#include "codec/codec.h"
#include "transaction_reference.h"

namespace token{
  typedef std::shared_ptr<Input> InputPtr;

  class Input : public Object{
   private:
    internal::proto::Input raw_;
   public:
    Input():
      Object(),
      raw_(){}
    explicit Input(internal::proto::Input raw):
      Object(),
      raw_(std::move(raw)){}
    explicit Input(const internal::BufferPtr& data):
      Input(){
      if(!raw_.ParseFromArray(data->data(), static_cast<int>(data->length())))
        DLOG(FATAL) << "cannot parse Input from buffer.";
    }
    Input(const Hash& tx, int64_t index, const std::string& user):
      Input(){
      raw_.set_user(user);
      raw_.set_hash(tx.HexString());
      raw_.set_index(index);
    }
    Input(const Hash& tx, int64_t index, const User& user):
      Input(){
      raw_.set_user(user.ToString());
      raw_.set_hash(tx.HexString());
      raw_.set_index(index);
    }
    Input(const Input& other) = default;
    ~Input() override = default;

    Type type() const override{
      return Type::kInput;
    }

    Hash transaction() const{
      return Hash::FromHexString(raw_.hash());
    }

    uint64_t index() const{
      return raw_.index();
    }

    User user() const{//TODO: change to std::string?
      return User(raw_.user());
    }

    internal::BufferPtr ToBuffer() const;

    /**
     * Returns the description of this object.
     *
     * @see Object::ToString()
     * @return The ToString() description of this object
     */
    std::string ToString() const override;

    Input& operator=(const Input& other) = default;

    friend bool operator==(const Input& a, const Input& b){
      return a.transaction() == b.transaction()
          && a.index() == b.index()
          && a.user() == b.user();
    }

    friend bool operator!=(const Input& a, const Input& b){
      return !operator==(a, b);
    }

    friend bool operator<(const Input& a, const Input& b){
      return a.transaction() < b.transaction();
    }

    friend bool operator>(const Input& a, const Input& b){
      return a.transaction() > b.transaction();
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