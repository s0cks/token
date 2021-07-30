#ifndef TOKEN_TRANSACTION_REFERENCE_H
#define TOKEN_TRANSACTION_REFERENCE_H

#include <utility>

#include "hash.h"
#include "json.h"
#include "object.h"

namespace token{
  class TransactionReference : public Object{
  private:
    Hash hash_;
    uint64_t index_;
   public:
    TransactionReference():
      Object(),
      hash_(),
      index_(){}
    TransactionReference(const Hash& tx, const uint64_t& index):
      Object(),
      hash_(tx),
      index_(index){}
    TransactionReference(const TransactionReference& other) = default;
    ~TransactionReference() override = default;

    Type type() const override{
      return Type::kTransactionReference;
    }

    Hash transaction() const{
      return hash_;
    }

    uint64_t index() const{
      return index_;
    }

    std::string ToString() const override{
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