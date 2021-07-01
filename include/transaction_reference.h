#ifndef TOKEN_TRANSACTION_REFERENCE_H
#define TOKEN_TRANSACTION_REFERENCE_H

#include "binary_object.h"

namespace token{
  static const int64_t kTransactionReferenceSize = Hash::kSize + sizeof(int64_t);
  class TransactionReference : public BinaryType<kTransactionReferenceSize>{
   private:
    enum Layout{
      kHashPosition = 0,
      kBytesForHash = Hash::kSize,

      kIndexPosition = kHashPosition+kBytesForHash,
      kBytesForIndex = sizeof(int64_t),
    };

    inline void
    SetHash(const Hash& val){
      memcpy(&data_[kHashPosition], val.data(), kBytesForHash);
    }

    inline void
    SetIndex(const int64_t& val){
      memcpy(&data_[kIndexPosition], &val, kBytesForIndex);
    }
   public:
    TransactionReference():
      BinaryType<kTransactionReferenceSize>(){}
    TransactionReference(const Hash& hash, const int64_t& index):
      BinaryType<kTransactionReferenceSize>(){
      SetHash(hash);
      SetIndex(index);
    }
    TransactionReference(const TransactionReference& other) = default;
    ~TransactionReference() override = default;

    Hash transaction() const{
      return Hash(&data_[kHashPosition], kBytesForHash);
    }

    int64_t index() const{
      return *((int64_t*)&data_[kIndexPosition]);
    }

    std::string ToString() const override{
      std::stringstream ss;
      ss << "TransactionReference(" << data() << ")";
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
}

#endif//TOKEN_TRANSACTION_REFERENCE_H