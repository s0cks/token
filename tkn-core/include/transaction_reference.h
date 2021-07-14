#ifndef TOKEN_TRANSACTION_REFERENCE_H
#define TOKEN_TRANSACTION_REFERENCE_H

#include "hash.h"
#include "json.h"
#include "object.h"

namespace token{
  class TransactionReference : public Object{
   private:
    Hash transaction_;
    int64_t index_;
   public:
    TransactionReference():
      transaction_(),
      index_(){}
    TransactionReference(const Hash& tx, const int64_t& index):
      transaction_(tx),
      index_(index){}
    TransactionReference(const TransactionReference& other) = default;
    ~TransactionReference() override = default;

    Type type() const override{
      return Type::kTransactionReference;
    }

    Hash& transaction(){
      return transaction_;
    }

    Hash transaction() const{
      return transaction_;
    }

    int64_t& index(){
      return index_;
    }

    int64_t index() const{
      return index_;
    }

    std::string ToString() const{
      std::stringstream ss;
      ss << "TransactionReference(";
      ss << "transaction=" << transaction() << ", ";
      ss << "index=" << index();
      ss << ")";
      return ss.str();
    }

    TransactionReference& operator=(const TransactionReference& other) = default;

    friend bool operator==(const TransactionReference& a, const TransactionReference& b){
      return Compare(a, b) == 0;
    }

    friend bool operator!=(const TransactionReference& a, const TransactionReference& b){
      return Compare(a, b) != 0;
    }

    friend bool operator<(const TransactionReference& a, const TransactionReference& b){
      return Compare(a, b) < 0;
    }

    friend bool operator>(const TransactionReference& a, const TransactionReference& b){
      return Compare(a, b) > 0;
    }

    friend std::ostream& operator<<(std::ostream& stream, const TransactionReference& val){
      return stream << val.ToString();
    }

    static inline int
    Compare(const TransactionReference& a, const TransactionReference& b){
      if(a.transaction() < b.transaction()){
        return -1;
      } else if(a.transaction() > b.transaction()){
        return +1;
      }

      if(a.index() < b.index()){
        return -1;
      } else if(a.index() > b.index()){
        return +1;
      }
      return 0;
    }
  };

  namespace codec{
    class TransactionReferenceEncoder : public codec::EncoderBase<TransactionReference>{
     public:
      TransactionReferenceEncoder(const TransactionReference& value, const codec::EncoderFlags& flags):
          codec::EncoderBase<TransactionReference>(value, flags){}
      TransactionReferenceEncoder(const TransactionReferenceEncoder& other) = default;
      ~TransactionReferenceEncoder() override = default;

      int64_t GetBufferSize() const override;
      bool Encode(const BufferPtr& buff) const override;
      TransactionReferenceEncoder& operator=(const TransactionReferenceEncoder& other) = default;
    };

    class TransactionReferenceDecoder : public codec::DecoderBase<TransactionReference>{
     public:
      explicit TransactionReferenceDecoder(const codec::DecoderHints& hints):
          codec::DecoderBase<TransactionReference>(hints){}
      TransactionReferenceDecoder(const TransactionReferenceDecoder& other) = default;
      ~TransactionReferenceDecoder() override = default;
      bool Decode(const BufferPtr& buff, TransactionReference& result) const override;
      TransactionReferenceDecoder& operator=(const TransactionReferenceDecoder& other) = default;
    };
  }

  namespace json{
    static inline bool
    Write(Writer& writer, const TransactionReference& val){
      JSON_START_OBJECT(writer);
      {
        if(!json::SetField(writer, "hash", val.transaction()))
          return false;
        if(!json::SetField(writer, "index", val.index()))
          return false;
      }
      JSON_END_OBJECT(writer);
      return true;
    }

    static inline bool
    SetField(Writer& writer, const char* name, const TransactionReference& val){
      JSON_KEY(writer, name);
      return Write(writer, val);
    }
  }
}

#endif//TOKEN_TRANSACTION_REFERENCE_H