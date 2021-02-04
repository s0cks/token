#ifndef TOKEN_UNCLAIMED_TRANSACTION_H
#define TOKEN_UNCLAIMED_TRANSACTION_H

#include "object.h"
#include "utils/buffer.h"

namespace token{
  class UnclaimedTransaction;
  typedef std::shared_ptr<UnclaimedTransaction> UnclaimedTransactionPtr;

  class Output;
  class Transaction;
  class UnclaimedTransaction : public BinaryObject{
   private:
    TransactionReference reference_;
    User user_;
    Product product_;
   public:
    UnclaimedTransaction(const TransactionReference& ref, const User& user, const Product& product):
      BinaryObject(),
      reference_(ref),
      user_(user),
      product_(product){}
    UnclaimedTransaction(const TransactionReference& ref, const std::string& user, const std::string& product):
      BinaryObject(),
      reference_(ref),
      user_(user),
      product_(product){}
    UnclaimedTransaction(const Hash& hash, int64_t index, const User& user, const Product& product):
      BinaryObject(),
      reference_(hash, index),
      user_(user),
      product_(product){}
    UnclaimedTransaction(const Hash& hash, int32_t index, const std::string& user, const std::string& product):
      BinaryObject(),
      reference_(hash, index),
      user_(user),
      product_(product){}
    ~UnclaimedTransaction(){}

    Type GetType() const{
      return Type::kUnclaimedTransaction;
    }

    TransactionReference& GetReference(){
      return reference_;
    }

    TransactionReference GetReference() const{
      return reference_;
    }

    User GetUser() const{
      return user_;
    }

    Product GetProduct() const{
      return product_;
    }

    int64_t GetBufferSize() const{
      return TransactionReference::kSize
           + User::GetSize()
           + Product::GetSize();
    }

    bool Write(const BufferPtr& buff) const{
      return buff->PutReference(reference_)
          && buff->PutUser(user_)
          && buff->PutProduct(product_);
    }

    bool Write(Json::Writer& writer) const{
      return writer.StartObject()
          && reference_.Write(writer)
          && user_.Write(writer)
          && product_.Write(writer)
          && writer.EndObject();
    }

    bool Equals(const UnclaimedTransactionPtr& val) const{
      return reference_ == val->reference_
          && user_ == val->user_
          && product_ == val->product_;
    }

    std::string ToString() const{
      std::stringstream stream;
      stream << "UnclaimedTransaction(" << reference_ << ", " << user_ << ", " << product_ << ")";
      return stream.str();
    }

    static inline UnclaimedTransactionPtr
    NewInstance(const TransactionReference& ref, const User& user, const Product& product){
      return std::make_shared<UnclaimedTransaction>(ref, user, product);
    }

    static inline UnclaimedTransactionPtr
    NewInstance(const TransactionReference& ref, const std::string& user, const std::string& product){
      return std::make_shared<UnclaimedTransaction>(ref, user, product);
    }

    static inline UnclaimedTransactionPtr
    NewInstance(const Hash& hash, int64_t index, const User& user, const Product& product){
      return std::make_shared<UnclaimedTransaction>(hash, index, user, product);
    }

    static inline UnclaimedTransactionPtr
    NewInstance(const Hash& hash, int32_t index, const std::string& user, const std::string& product){
      return std::make_shared<UnclaimedTransaction>(hash, index, user, product);
    }

    static inline UnclaimedTransactionPtr
    FromBytes(const BufferPtr& buff){
      TransactionReference ref = buff->GetReference();
      User user = buff->GetUser();
      Product product = buff->GetProduct();
      return std::make_shared<UnclaimedTransaction>(ref, user, product);
    }
  };
}

#endif //TOKEN_UNCLAIMED_TRANSACTION_H