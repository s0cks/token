#ifndef TOKEN_TRANSACTION_OUTPUT_H
#define TOKEN_TRANSACTION_OUTPUT_H

#include <vector>
#include "user.h"
#include "product.h"

namespace token{
  class Output : public SerializableObject{
    friend class Transaction;
   public:
    static inline int64_t
    GetSize(){
      return User::GetSize()
          + Product::GetSize();
    }
   private:
    User user_;
    Product product_;
   public:
    Output(const User& user, const Product& product):
      SerializableObject(),
      user_(user),
      product_(product){}
    Output(const std::string& user, const Product& product):
      Output(User(user), product){}
    Output(const std::string& user, const std::string& product):
      Output(User(user), Product(product)){}
    Output(const char* user, const char* product):
      Output(User(user), Product(product)){}
    explicit Output(const BufferPtr& buff);
    Output(const Output& other) = default;
    ~Output() override = default;

    Type GetType() const override{
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
     * Returns the size of this object (in bytes).
     *
     * @return The size of this encoded object in bytes
     */
    int64_t GetBufferSize() const override{
      return User::GetSize()
          + Product::GetSize();
    }

    /**
     * Serializes this object to a Buffer.
     *
     * @param buffer - The Buffer to write to
     * @see Buffer
     * @return True when successful otherwise, false
     */
    bool Write(const BufferPtr& buff) const override;

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

    static Output ParseFrom(const BufferPtr& buff);
  };

  typedef std::vector<Output> OutputList;
}

#endif//TOKEN_TRANSACTION_OUTPUT_H