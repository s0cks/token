#ifndef TOKEN_TRANSACTION_OUTPUT_H
#define TOKEN_TRANSACTION_OUTPUT_H

#include <vector>
#include <glog/logging.h>

#include "token/user.h"
#include "token/object.h"
#include "token/product.h"

namespace token{
 class Output;
 class OutputVisitor{
  protected:
   OutputVisitor() = default;
  public:
   virtual ~OutputVisitor() = default;
   virtual void Visit(const Output& val) = 0;
 };

 class Output : public SerializableObject{
  public:
   static constexpr const uint64_t kSize = User::kSize + Product::kSize;

   static inline int
   Compare(const Output& lhs, const Output& rhs){
     if(lhs.user() < rhs.user()){
       return -1;
     } else if(lhs.user() > rhs.user()){
       return +1;
     }

     if(lhs.product() < rhs.product()){
       return -1;
     } else if(lhs.product() > rhs.product()){
       return +1;
     }
     return 0;
   }
  private:
   User user_;
   Product product_;
  public:
   Output():
     user_(),
     product_(){
   }
   Output(const User& user, const Product& product):
     user_(user),
     product_(product){
   }
   Output(const std::string& user, const std::string& product):
    Output(User(user), Product(product)){
   }
   explicit Output(const BufferPtr& data):
    user_(data->GetUser()),
    product_(data->GetProduct()){
   }
   Output(const Output& rhs):
    user_(rhs.user()),
    product_(rhs.product()){
   }
   ~Output() = default;

   Type type() const override{
     return Type::kOutput;
   }

   User user() const{
     return user_;
   }

   Product product() const{
     return product_;
   }

   uint64_t GetBufferSize() const override{
     return kSize;
   }

   bool WriteTo(const BufferPtr& data) const override{
     return data->PutUser(user_)
         && data->PutProduct(product_);
   }

   Output& operator=(const Output& rhs){
     if(&rhs == this)
       return *this;
     user_ = rhs.user();
     product_ = rhs.product();
     return *this;
   }

   friend std::ostream& operator<<(std::ostream& stream, const Output& rhs){
     return stream << "Output(user=" << rhs.user() << ", product=" << rhs.product() << ")";
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
 };
}

#endif//TOKEN_TRANSACTION_OUTPUT_H