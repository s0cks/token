#ifndef TOKEN_TRANSACTION_OUTPUT_H
#define TOKEN_TRANSACTION_OUTPUT_H

#include <vector>

#include "output.pb.h"

#include "object.h"
#include "type/user.h"
#include "type/product.h"

namespace token{
  class Output : public Object{
   private:
    internal::proto::Output raw_;
   public:
    Output():
      Object(),
      raw_(){}
    explicit Output(internal::proto::Output raw):
      Object(),
      raw_(std::move(raw)){}
    explicit Output(const internal::BufferPtr& data):
      Output(){
      if(!raw_.ParseFromArray(data->data(), static_cast<int>(data->length())))
        DLOG(FATAL) << "cannot parse Output from buffer.";
    }
    Output(const std::string& user, const std::string& product):
      Output(){
      raw_.set_user(user);
      raw_.set_product(product);
    }
    Output(const Output& other) = default;
    ~Output() override = default;

    Type type() const override{
      return Type::kOutput;
    }

    User user() const{
      return User(raw_.user());
    }

    Product product() const{
      return Product(raw_.product());
    }

    internal::BufferPtr ToBuffer() const;

    /**
     * Returns the description of this object.
     *
     * @see Object::ToString()
     * @return The ToString() description of this object
     */
    std::string ToString() const override;

    Output& operator=(const Output& other) = default;

    friend bool operator==(const Output& a, const Output& b){
      return a.user() == b.user()
          && a.product() == b.product();
    }

    friend bool operator!=(const Output& a, const Output& b){
      return !operator==(a, b);
    }

    friend bool operator<(const Output& a, const Output& b){
      NOT_IMPLEMENTED(ERROR);
      return true;
    }

    friend bool operator>(const Output& a, const Output& b){
      NOT_IMPLEMENTED(ERROR);
      return true;
    }

    friend std::ostream& operator<<(std::ostream& stream, const Output& output){
      return stream << output.ToString();
    }
  };

  namespace json{
    static inline bool
    Write(Writer& writer, const Output& val){
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