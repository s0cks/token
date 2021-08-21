#ifndef TOKEN_UNCLAIMED_TRANSACTION_H
#define TOKEN_UNCLAIMED_TRANSACTION_H

#include "unclaimed_transaction.pb.h"

#include "json.h"
#include "object.h"
#include "builder.h"
#include "type/user.h"
#include "type/product.h"
#include "transaction_reference.h"

namespace token{
  class UnclaimedTransactionVisitor{
  protected:
    UnclaimedTransactionVisitor() = default;
  public:
    virtual ~UnclaimedTransactionVisitor() = default;
    virtual bool Visit(const UnclaimedTransactionPtr& val) const = 0;
  };

  class UnclaimedTransaction: public Object{
  public:
    class Builder : public internal::ProtoBuilder<UnclaimedTransaction, internal::proto::UnclaimedTransaction>{
    public:
      explicit Builder(internal::proto::UnclaimedTransaction* raw):
        internal::ProtoBuilder<UnclaimedTransaction, internal::proto::UnclaimedTransaction>(raw){}
      Builder() = default;
      ~Builder() override = default;

      void SetTransactionHash(const Hash& val){
        return raw_->set_transaction(val.HexString());
      }

      void SetTransactionIndex(const uint64_t& val){
        return raw_->set_index(val);
      }

      void SetUser(const std::string& user){
        return raw_->set_user(user);
      }

      void SetProduct(const std::string& product){
        return raw_->set_product(product);
      }

      UnclaimedTransactionPtr Build() const override{
        return std::make_shared<UnclaimedTransaction>(*raw_);
      }

      Builder& operator=(const Builder& rhs) = default;
    };
  private:
    internal::proto::UnclaimedTransaction raw_;
  public:
    UnclaimedTransaction():
      Object(),
      raw_(){}
    explicit UnclaimedTransaction(internal::proto::UnclaimedTransaction raw):
      Object(),
      raw_(std::move(raw)){}
    explicit UnclaimedTransaction(const internal::BufferPtr& data):
      UnclaimedTransaction(){
      if(!raw_.ParseFromArray(data->data(), static_cast<int>(data->length())))
        DLOG(FATAL) << "cannot parse UnclaimedTransaction from buffer.";
    }
    UnclaimedTransaction(const UnclaimedTransaction& other) = default;
    ~UnclaimedTransaction() override = default;

    Type type() const override{
      return Type::kUnclaimedTransaction;
    }

    Hash transaction() const{
      return Hash::FromHexString(raw_.transaction());
    }

    uint64_t index() const{
      return raw_.index();
    }

    User user() const{
      return User(raw_.user());
    }

    Product product() const{
      return Product(raw_.product());
    }

    Hash hash() const{
      auto data = ToBuffer();
      return Hash::ComputeHash<CryptoPP::SHA256>(data->data(), data->length());
    }

    BufferPtr ToBuffer() const;
    std::string ToString() const override;
    UnclaimedTransaction& operator=(const UnclaimedTransaction& other) = default;

    static inline UnclaimedTransactionPtr
    NewInstance(internal::proto::UnclaimedTransaction raw){
      return std::make_shared<UnclaimedTransaction>(std::move(raw));
    }

    static inline UnclaimedTransactionPtr
    From(const internal::BufferPtr& data){
      return std::make_shared<UnclaimedTransaction>(data);
    }
  };

  namespace json{
    static inline bool
    Write(Writer& writer, const UnclaimedTransactionPtr& val){
      NOT_IMPLEMENTED(FATAL);
      return true;
    }

    static inline bool
    SetField(Writer& writer, const char* name, const UnclaimedTransactionPtr& val){
      JSON_KEY(writer, name);
      return Write(writer, val);
    }
  }
}
#endif //TOKEN_UNCLAIMED_TRANSACTION_H