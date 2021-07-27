#ifndef TOKEN_UNCLAIMED_TRANSACTION_H
#define TOKEN_UNCLAIMED_TRANSACTION_H

#include "object.h"
#include "type/user.h"
#include "type/product.h"
#include "transaction_reference.h"
#include "json.h"
#include "codec/codec.h"

namespace token{
  class UnclaimedTransaction: public Object{
  public:
    class Encoder: public codec::TypeEncoder<UnclaimedTransaction>{
    protected:
      TransactionReference::Encoder encode_txref_;
      User::Encoder encode_user_;
      Product::Encoder encode_product_;
    public:
      explicit Encoder(const UnclaimedTransaction* value, const codec::EncoderFlags& flags);
      Encoder(const Encoder& other) = default;
      ~Encoder() override = default;
      int64_t GetBufferSize() const override;
      bool Encode(const BufferPtr& buff) const override;
      Encoder& operator=(const Encoder& other) = default;
    };

    class Decoder: public codec::DecoderBase<UnclaimedTransaction>{
    protected:
      TransactionReference::Decoder decode_txref_;
      User::Decoder decode_user_;
      Product::Decoder decode_product_;
    public:
      explicit Decoder(const codec::DecoderHints& hints = codec::kDefaultDecoderHints);
      Decoder(const Decoder& other) = default;
      ~Decoder() override = default;
      bool Decode(const BufferPtr& buff, UnclaimedTransaction& result) const override;
      Decoder& operator=(const Decoder& other) = default;
    };
  private:
    TransactionReference reference_;
    User user_;
    Product product_;
  public:
    UnclaimedTransaction() = default;
    UnclaimedTransaction(const TransactionReference& ref, const User& user, const Product& product):
      Object(),
      reference_(ref),
      user_(user),
      product_(product){}
    UnclaimedTransaction(const TransactionReference& ref, const std::string& user, const std::string& product):
      Object(),
      reference_(ref),
      user_(user),
      product_(product){}
    UnclaimedTransaction(const Hash& hash, int64_t index, const User& user, const Product& product):
      Object(),
      reference_(hash, index),
      user_(user),
      product_(product){}
    UnclaimedTransaction(const Hash& hash, int32_t index, const std::string& user, const std::string& product):
      Object(),
      reference_(hash, index),
      user_(user),
      product_(product){}
    UnclaimedTransaction(const UnclaimedTransaction& other) = default;
    ~UnclaimedTransaction() override = default;

    Type type() const override{
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

    Hash hash() const{
      Encoder encoder((UnclaimedTransaction*)this, codec::kDefaultEncoderFlags);
      internal::BufferPtr data = internal::NewBufferFor(encoder);
      if(!encoder.Encode(data)){
        LOG(ERROR) << "cannot encoder Block to buffer.";
        return Hash();
      }
      return Hash::ComputeHash<CryptoPP::SHA256>(data->data(), data->GetWritePosition());
    }

    BufferPtr ToBuffer() const;
    std::string ToString() const override;
    UnclaimedTransaction& operator=(const UnclaimedTransaction& other) = default;

    friend bool operator==(const UnclaimedTransaction& a, const UnclaimedTransaction& b){
      return a.reference_ == b.reference_
             && a.user_ == b.user_
             && a.product_ == b.product_;
    }

    friend bool operator!=(const UnclaimedTransaction& a, const UnclaimedTransaction& b){
      return !operator==(a, b);
    }

    //TODO:
    // - implement: <
    // - implement: >

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

    static inline bool
    Decode(const BufferPtr& buff, UnclaimedTransaction& result, const codec::DecoderHints& hints = codec::kDefaultDecoderHints){
      Decoder decoder(hints);
      return decoder.Decode(buff, result);
    }

    static inline UnclaimedTransactionPtr
    DecodeNew(const BufferPtr& buff, const codec::DecoderHints& hints = codec::kDefaultDecoderHints){
      UnclaimedTransaction result;
      if(!Decode(buff, result, hints)){
        DLOG(ERROR) << "cannot decode UnclaimedTransaction.";
        return nullptr;
      }
      return std::make_shared<UnclaimedTransaction>(result);
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