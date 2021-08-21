#ifndef TOKEN_TRANSACTION_OUTPUT_H
#define TOKEN_TRANSACTION_OUTPUT_H

#include <vector>

#include "output.pb.h"

#include "object.h"
#include "type/user.h"
#include "type/product.h"

namespace token{
  class OutputVisitor{
  protected:
    OutputVisitor() = default;
  public:
    virtual ~OutputVisitor() = default;
    virtual bool Visit(const OutputPtr& val) = 0;
  };

  typedef internal::proto::Output RawOutput;
  class Output : public Object{
  public:
    static inline int
    Compare(const Output& lhs, const Output& rhs){
      NOT_IMPLEMENTED(FATAL);//TODO: implement
      return 0;
    }
  private:
    User user_;
    Product product_;
  public:
    Output():
      Object(),
      user_(),
      product_(){
    }
    Output(const User& user, const Product& product):
      Object(),
      user_(user),
      product_(product){
    }
    explicit Output(const RawOutput& val):
      Object(),
      user_(val.user()),
      product_(val.product()){
    }
    Output(const Output& rhs) = default;
    ~Output() override = default;

    Type type() const override{
      return Type::kOutput;
    }

    User user() const{
      return user_;
    }

    Product product() const{
      return product_;
    }

    internal::BufferPtr ToBuffer() const;

    std::string ToString() const override{
      std::stringstream ss;
      ss << "Output(";
      ss << "user=" << user() << ", ";
      ss << "product=" << product();
      ss << ")";
      return ss.str();
    }

    Output& operator=(const Output& rhs) = default;

    Output& operator=(const RawOutput& rhs){
      user_ = User(rhs.user());
      product_ = Product(rhs.product());
      return *this;
    }

    friend std::ostream& operator<<(std::ostream& stream, const Output& rhs){
      return stream << rhs.ToString();
    }

    friend bool operator==(const Output& lhs, const Output& rhs){
      return Compare(lhs, rhs) == 0;
    }

    friend bool operator!=(const Output& lhs, const Output& rhs){
      return Compare(lhs, rhs) != 0;
    }

    friend bool operator<(const Output& lhs, const Output& rhs){
      return Compare(lhs, rhs) < 0;
    }

    friend bool operator>(const Output& lhs, const Output& rhs){
      return Compare(lhs, rhs) > 0;
    }

    static inline OutputPtr
    NewInstance(const User& user, const Product& product){
      return std::make_shared<Output>(user, product);
    }

    static inline OutputPtr
    From(const RawOutput& val){
      return std::make_shared<Output>(val);
    }

    static inline OutputPtr
    From(const internal::BufferPtr& val){
      auto length = val->GetUnsignedLong();
      DVLOG(2) << "decoded Output length: " << length;
      RawOutput raw;
      if(!val->GetMessage(raw, length)){
        LOG(FATAL) << "cannot decode Output (" << length << "b) from buffer of size: " << val->length();
        return nullptr;
      }
      return From(raw);
    }

    static inline OutputPtr
    CopyFrom(const Output& val){
      return std::make_shared<Output>(val);
    }

    static inline OutputPtr
    CopyFrom(const OutputPtr& val){
      return NewInstance(val->user(), val->product());
    }
  };

  static inline RawOutput&
  operator<<(RawOutput& raw, const Output& val){
    raw.set_user(val.user().data());
    raw.set_product(val.product().data());
    return raw;
  }

  static inline RawOutput&
  operator<<(RawOutput& raw, const OutputPtr& val){
    return raw << (*val);
  }

  typedef std::vector<OutputPtr> OutputList;

  static inline OutputList&
  operator<<(OutputList& list, const OutputPtr& val){
    list.push_back(val);
    return list;
  }

  static inline OutputList&
  operator<<(OutputList& list, const Output& val){
    return list << Output::CopyFrom(val);
  }

  namespace json{
    static inline bool
    Write(Writer& writer, const Output& val){//TODO: better error handling
      JSON_START_OBJECT(writer);
      {
        if(!json::SetField(writer, "user", val.user()))
          return false;
        if(!json::SetField(writer, "product", val.product()))
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
  }
}

#endif//TOKEN_TRANSACTION_OUTPUT_H