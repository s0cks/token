#ifndef TOKEN_KEY_H
#define TOKEN_KEY_H

#include "hash.h"
#include "object.h"

namespace token{
  class KeyBase{
   protected:
    KeyBase() = default;
   public:
    KeyBase(const KeyBase& other) = default;
    virtual ~KeyBase() = default;
    virtual Type type() const = 0;
    virtual size_t size() const = 0;
    virtual char* data() const = 0;
    virtual std::string ToString() const = 0;
    KeyBase& operator=(const KeyBase& other) = default;

    operator leveldb::Slice() const{
      return leveldb::Slice(data(), size());
    }
  };

  class ObjectKey : public KeyBase{
   protected:
    enum KeyLayout{
      // type
      kTypePosition = 0,
      kBytesForType = sizeof(uint32_t),
      // hash
      kHashPosition = kTypePosition + kBytesForType,
      kBytesForHash = Hash::kSize,
      // total
      kTotalSize = kHashPosition + kBytesForHash,
    };

    uint8_t data_[kTotalSize];

    inline void
    ClearData(){
      memset(data_, 0, kTotalSize);
    }

    inline void
    SetType(const Type& type){
      uint32_t value = static_cast<uint32_t>(type);
      memcpy(&data_[kTypePosition], &value, kBytesForType);
    }

    inline void
    SetHash(const Hash& hash){
      memcpy(&data_[kHashPosition], hash.data(), kBytesForHash);
    }
   public:
    ObjectKey(const Type& type, const Hash& hash):
      KeyBase(),
      data_(){
      ClearData();
      SetType(type);
      SetHash(hash);
    }
    ObjectKey(const leveldb::Slice& slice):
      KeyBase(),
      data_(){
      ClearData();
      assert(slice.size() == kTotalSize);
      memcpy(data_, slice.data(), slice.size());
    }
    ObjectKey(const ObjectKey& other):
      KeyBase(),
      data_(){
      ClearData();
      memcpy(data_, other.data_, kTotalSize);
    }
    virtual ~ObjectKey() override = default;

    Type type() const override{
      return *(Type*) &data_[kTypePosition];
    }

    Hash hash() const{
      return Hash(&data_[kHashPosition], kBytesForHash);
    }

    size_t size() const override{
      return kTotalSize;
    }

    char* data() const override{
      return (char*) data_;
    }

    std::string ToString() const override{
      std::stringstream ss;
      ss << type() << "Key(";
      ss << ")";
      return ss.str();
    }

    ObjectKey& operator=(const ObjectKey& other) = default;

    friend std::ostream& operator<<(std::ostream& stream, const ObjectKey& key){
      return stream << key.ToString();
    }

    friend bool operator==(const ObjectKey& a, const ObjectKey& b){
      return memcmp(a.data_, b.data_, kTotalSize) == 0;
    }

    friend bool operator!=(const ObjectKey& a, const ObjectKey& b){
      return memcmp(a.data_, b.data_, kTotalSize) != 0;
    }

    friend bool operator<(const ObjectKey& a, const ObjectKey& b){
      if (a.type() == b.type())
        return a.hash() < b.hash();
      return a.type() < b.type();
    }

    friend bool operator>(const ObjectKey& a, const ObjectKey& b){
      if (a.type() == b.type())
        return a.hash() > b.hash();
      return a.type() > b.type();
    }

    static inline int
    Compare(const ObjectKey& a, const ObjectKey& b){
      Type t1 = a.type(), t2 = b.type();
      if (t1 < t2){
        return -1;
      } else if (t1 > t2){
        return +1;
      }

      Hash h1 = a.hash(), h2 = b.hash();
      if (h1 < h2){
        return -1;
      } else if (h1 > h2){
        return +1;
      }
      return 0;
    }
  };
}

#endif//TOKEN_KEY_H