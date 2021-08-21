#ifndef TOKEN_TRANSACTION_INPUT_H
#define TOKEN_TRANSACTION_INPUT_H

#include <vector>
#include "hash.h"
#include "input.pb.h"
#include "transaction_reference.h"

namespace token{
  class InputVisitor{
  protected:
    InputVisitor() = default;
  public:
    virtual ~InputVisitor() = default;
    virtual bool Visit(const InputPtr& val) = 0;
  };

  typedef internal::proto::Input RawInput;
  class Input : public Object{
  public:
    static inline int
    Compare(const Input& lhs, const Input& rhs){
      if(lhs.hash() < rhs.hash()){
        return -1;
      } else if(lhs.hash() > rhs.hash()){
        return +1;
      }
      return 0;
    }
  private:
    Hash hash_;
    TransactionReference source_;
  public:
    Input():
      Object(),
      hash_(),
      source_(){
    }
    Input(const Hash& hash, const TransactionReference& source):
      Object(),
      hash_(hash),
      source_(source){
    }
    explicit Input(const RawInput& val):
      Object(),
      hash_(Hash::FromHexString(val.hash())),
      source_(Hash::FromHexString(val.transaction()), val.index()){
    }
    Input(const Input& rhs) = default;
    ~Input() override = default;

    Type type() const override{
      return Type::kInput;
    }

    Hash hash() const{
      return hash_;
    }

    TransactionReference source() const{
      return source_;
    }

    std::string ToString() const override{
      std::stringstream ss;
      ss << "Input(";
      ss << "hash=" << hash() << ", ";
      ss << "source=" << source();
      ss << ")";
      return ss.str();
    }

    internal::BufferPtr ToBuffer() const;

    Input& operator=(const Input& rhs) = default;

    Input& operator=(const RawInput& rhs){
      hash_ = Hash::FromHexString(rhs.hash());
      source_ = TransactionReference(Hash::FromHexString(rhs.transaction()), rhs.index());
      return *this;
    }

    friend std::ostream& operator<<(std::ostream& stream, const Input& rhs){
      return stream << rhs.ToString();
    }

    friend bool operator==(const Input& lhs, const Input& rhs){
      return Compare(lhs, rhs) == 0;
    }

    friend bool operator!=(const Input& lhs, const Input& rhs){
      return Compare(lhs, rhs) != 0;
    }

    friend bool operator<(const Input& lhs, const Input& rhs){
      return Compare(lhs, rhs) < 0;
    }

    friend bool operator>(const Input& lhs, const Input& rhs){
      return Compare(lhs, rhs) > 0;
    }

    static inline InputPtr
    NewInstance(const Hash& utxo, const TransactionReference& source){
      return std::make_shared<Input>(utxo, source);
    }

    static inline InputPtr
    From(const RawInput& val){
      return std::make_shared<Input>(val);
    }

    static inline InputPtr
    From(const internal::BufferPtr& val){
      auto length = val->GetUnsignedLong();
      DVLOG(2) << "decoded Input length: " << length;
      RawInput raw;
      if(!val->GetMessage(raw, length)){
        LOG(FATAL) << "cannot decode Input (" << length << "b) from buffer of size: " << val->length();
        return nullptr;
      }
      return From(raw);
    }

    static inline InputPtr
    CopyFrom(const Input& val){
      return std::make_shared<Input>(val);
    }

    static inline InputPtr
    CopyFrom(const InputPtr& val){
      return NewInstance(val->hash(), val->source());
    }
  };

  static inline RawInput&
  operator<<(RawInput& raw, const Input& val){
    raw.set_hash(val.hash().HexString());
    auto source = val.source();
    raw.set_transaction(source.transaction().HexString());
    raw.set_index(source.index());
    return raw;
  }

  static inline RawInput&
  operator<<(RawInput& raw, const InputPtr& val){
    return raw << (*val);
  }

  typedef std::vector<InputPtr> InputList;

  static inline InputList&
  operator<<(InputList& list, const InputPtr& val){
    list.push_back(val);
    return list;
  }

  static inline InputList&
  operator<<(InputList& list, const Input& val){
    return list << Input::CopyFrom(val);
  }

namespace json{
    static inline bool
    Write(Writer& writer, const Input& val){//TODO: better error handling
      JSON_START_OBJECT(writer);
      {
        JSON_KEY(writer, "hash");
        JSON_STRING(writer, val.hash().HexString());
        if(!json::SetField(writer, "source", val.source()))
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
  }
}

#endif//TOKEN_TRANSACTION_INPUT_H