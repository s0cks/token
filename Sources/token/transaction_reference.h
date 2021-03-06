#ifndef TOKEN_TRANSACTION_REFERENCE_H
#define TOKEN_TRANSACTION_REFERENCE_H

#include "hash.h"

namespace token{
 class TransactionReference{
  public:
   static constexpr const uint64_t kSize = uint256::kSize + sizeof(uint64_t);

   static inline int
   Compare(const TransactionReference& lhs, const TransactionReference& rhs){
     if(lhs.hash() < rhs.hash()){
       return -1;
     } else if(lhs.hash() > rhs.hash()){
       return +1;
     }

     if(lhs.index() < rhs.index()){
       return -1;
     } else if(lhs.index() > rhs.index()){
       return +1;
     }
     return 0;
   }
  private:
   uint256 hash_;
   uint64_t index_;
  public:
   TransactionReference():
    hash_(),
    index_(){
   }
   TransactionReference(const uint256& hash, uint64_t index):
    hash_(hash),
    index_(index){
   }
   TransactionReference(const TransactionReference& rhs):
    hash_(rhs.hash()),
    index_(rhs.index()){
   }
   ~TransactionReference() = default;

   uint256 hash() const{
     return hash_;
   }

   uint64_t index() const{
     return index_;
   }

   TransactionReference& operator=(const TransactionReference& rhs){
     if(&rhs == this)
       return *this;
     hash_ = rhs.hash();
     index_ = rhs.index();
     return *this;
   }

   friend std::ostream& operator<<(std::ostream& stream, const TransactionReference& val){
     return stream << "TransactionReference(hash=" << val.hash() << ", index=" << val.index() << ")";
   }

   friend bool operator==(const TransactionReference& lhs, const TransactionReference& rhs){
     return Compare(lhs, rhs) == 0;
   }

   friend bool operator!=(const TransactionReference& lhs, const TransactionReference& rhs){
     return Compare(lhs, rhs) != 0;
   }

   friend bool operator<(const TransactionReference& lhs, const TransactionReference& rhs){
     return Compare(lhs, rhs) < 0;
   }

   friend bool operator>(const TransactionReference& lhs, const TransactionReference& rhs){
     return Compare(lhs, rhs) > 0;
   }
 };
}

#endif//TOKEN_TRANSACTION_REFERENCE_H