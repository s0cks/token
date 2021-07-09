#ifndef TOKEN_REFERENCE_H
#define TOKEN_REFERENCE_H
#include "hash.h"
#include "flags.h"
#include "kvstore.h"
#include "binary_type.h"
#include "atomic/relaxed_atomic.h"

namespace token{
  static const int64_t kReferenceSize = 64;
  class Reference : public BinaryType<kReferenceSize>{
   public:
    Reference():
      BinaryType<kReferenceSize>(){}
    explicit Reference(const char* val):
      BinaryType<kReferenceSize>(){
      memcpy(data_, val, std::min((int64_t)strlen(val), kReferenceSize));
    }
    explicit Reference(const std::string& val):
      BinaryType<kReferenceSize>(){
      memcpy(data_, val.data(), std::min((int64_t)val.length(), kReferenceSize));
    }
    explicit Reference(const leveldb::Slice& val):
      BinaryType<kReferenceSize>(){
      memcpy(data_, val.data(), std::min((int64_t)val.size(), kReferenceSize));
    }
    Reference(const Reference& other) = default;
    ~Reference() override = default;

    std::string ToString() const override{
      std::stringstream ss;
      ss << "Reference(" << data() << ")";
      return ss.str();
    }

    Reference& operator=(const Reference& other) = default;

    friend std::ostream& operator<<(std::ostream& stream, const Reference& val){
      return stream << val.ToString();
    }

    explicit operator leveldb::Slice() const{
      return leveldb::Slice(data(), size());
    }
  };

  class ReferenceDatabase;
  typedef std::shared_ptr<ReferenceDatabase> ReferenceDatabasePtr;
}

#endif//TOKEN_REFERENCE_H