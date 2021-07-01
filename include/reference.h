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
    Reference(const char* val):
      BinaryType<kReferenceSize>(){
      memcpy(data_, val, std::min((int64_t)strlen(val), kReferenceSize));
    }
    Reference(const std::string& val):
      BinaryType<kReferenceSize>(){
      memcpy(data_, val.data(), std::min((int64_t)val.length(), kReferenceSize));
    }
    Reference(const leveldb::Slice& val):
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

    operator leveldb::Slice() const{
      return leveldb::Slice(data(), size());
    }
  };

  class ReferenceDatabase;
  typedef std::shared_ptr<ReferenceDatabase> ReferenceDatabasePtr;

  class ReferenceDatabase : public internal::KeyValueStore{
   public:
    class Comparator : public leveldb::Comparator{
      friend class ReferenceDatabase;
     public:
      Comparator() = default;
      ~Comparator() override = default;

      int Compare(const leveldb::Slice& a, const leveldb::Slice& b) const override{
        Reference r1(a), r2(b);
        return Reference::Compare(r1, r2);
      }

      const char* Name() const override{
        return "ReferenceComparator";
      }

      void FindShortestSeparator(std::string* str, const leveldb::Slice& slice) const override{}
      void FindShortSuccessor(std::string* str) const override{}
    };
   public:
    ReferenceDatabase(): internal::KeyValueStore(){}
    virtual ~ReferenceDatabase() override = default;

    virtual bool HasReference(const Reference& key) const;
    virtual bool PutReference(const Reference& key, const Hash& val) const;
    virtual bool RemoveReference(const Reference& key) const;
    virtual Hash GetReference(const Reference& key) const;
    virtual int64_t GetNumberOfReferences() const;

    inline bool
    HasReference(const char* key) const{
      return HasReference(Reference(key));
    }

    static inline std::string
    GetIndexFilename(){
      std::stringstream filename;
      filename << TOKEN_BLOCKCHAIN_HOME << "/refs";
      return filename.str();
    }

    static inline const char*
    GetName(){
      return "ReferenceDatabase";
    }

    static inline ReferenceDatabasePtr
    NewInstance(){
      return std::make_shared<ReferenceDatabase>();
    }

    static ReferenceDatabasePtr GetInstance();
    static bool Initialize(const std::string& filename=GetIndexFilename());
  };
}

#endif//TOKEN_REFERENCE_H