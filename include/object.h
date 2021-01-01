#ifndef TOKEN_OBJECT_H
#define TOKEN_OBJECT_H

#include <map>
#include <vector>
#include <string>
#include <leveldb/slice.h>
#include "hash.h"
#include "utils/bitfield.h"

namespace Token{
  class Object;

  class Object{
    friend class Thread;
    friend class BinaryFileWriter;
   public:
    virtual ~Object() = default;
    virtual std::string ToString() const = 0;
  };

  class Buffer;
  typedef std::shared_ptr<Buffer> BufferPtr;

  class BinaryObject : public Object{
   protected:
    BinaryObject() = default;
   public:
    virtual ~BinaryObject() = default;

    virtual int64_t GetBufferSize() const = 0;
    virtual bool Write(const BufferPtr &buff) const = 0;
    Hash GetHash() const;
  };

  template<int64_t Size>
  class RawType{
   protected:
    uint8_t data_[Size];

    RawType():
      data_(){
      memset(data_, 0, GetSize());
    }
    RawType(const uint8_t *bytes, int64_t size):
      data_(){
      memset(data_, 0, GetSize());
      memcpy(data_, bytes, std::min(size, GetSize()));
    }
    RawType(const RawType &raw):
      data_(){
      memset(data_, 0, GetSize());
      memcpy(data_, raw.data_, GetSize());
    }

    static inline int
    Compare(const RawType<Size> &a, const RawType<Size> &b){
      return memcmp(a.data(), b.data(), Size);
    }
   public:
    virtual ~RawType() = default;

    char *data() const{
      return (char *) data_;
    }

    std::string str() const{
      return std::string(data(), Size);
    }

    static inline int64_t
    GetSize(){
      return Size;
    }
  };

  class Product : public RawType<64>{
    using Base = RawType<64>;
   public:
    static const int64_t kSize = 64;

    Product():
      Base(){}
    Product(const uint8_t *bytes, int64_t size):
      Base(bytes, size){}
    Product(const Product &product):
      Base(){
      memcpy(data(), product.data(), Base::GetSize());
    }
    Product(const std::string &value):
      Base(){
      memcpy(data(), value.data(), std::min((int64_t) value.length(), Base::GetSize()));
    }
    ~Product() = default;

    int64_t size() const{
      return std::min((int64_t) strlen((char *) data_), Base::GetSize());
    }

    std::string Get() const{
      return std::string((char *) data_, Base::GetSize());
    }

    void operator=(const Product &product){
      memcpy(data(), product.data(), Base::GetSize());
    }

    friend bool operator==(const Product &a, const Product &b){
      return Base::Compare(a, b) == 0;
    }

    friend bool operator!=(const Product &a, const Product &b){
      return Base::Compare(a, b) != 0;
    }

    friend int operator<(const Product &a, const Product &b){
      return Base::Compare(a, b);
    }

    friend std::ostream &operator<<(std::ostream &stream, const Product &product){
      stream << product.str();
      return stream;
    }
  };

  class User : public RawType<64>{
    using Base = RawType<64>;
   public:
    static const int64_t kSize = 64;

    User():
      Base(){}
    User(const uint8_t *bytes, int64_t size):
      Base(bytes, size){}
    User(const User &user):
      Base(){
      memcpy(data(), user.data(), Base::GetSize());
    }
    User(const std::string &value):
      Base(){
      memcpy(data(), value.data(), std::min((int64_t) value.length(), Base::GetSize()));
    }
    ~User() = default;

    int64_t size() const{
      return std::min((int64_t) strlen((char *) data_), Base::GetSize());
    }

    std::string Get() const{
      return std::string((char *) data_, Base::GetSize());
    }

    void operator=(const User &user){
      memcpy(data(), user.data(), Base::GetSize());
    }

    friend bool operator==(const User &a, const User &b){
      return Base::Compare(a, b) == 0;
    }

    friend bool operator!=(const User &a, const User &b){
      return Base::Compare(a, b) != 0;
    }

    friend int operator<(const User &a, const User &b){
      return Base::Compare(a, b);
    }

    friend std::ostream &operator<<(std::ostream &stream, const User &user){
      stream << user.str();
      return stream;
    }
  };
}

#endif //TOKEN_OBJECT_H