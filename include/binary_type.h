#ifndef TOKEN_BINARY_TYPE_H
#define TOKEN_BINARY_TYPE_H

namespace token{
  template<uint64_t Size>
  class BinaryType{
   protected:
    uint8_t data_[Size];

    BinaryType():
      data_(){
      memset(data_, 0, Size);
    }
    BinaryType(const uint8_t* data, const uint64_t& size):
      data_(){
      memset(data_, 0, Size);
      memcpy(data_, data, std::min(size, Size));
    }
    explicit BinaryType(const BinaryType<Size>& other):
      data_(){
      memset(data_, 0, Size);
      memcpy(data_, other.data(), Size);
    }

    static inline int
    Compare(const BinaryType<Size>& a, const BinaryType<Size>& b){
      return memcmp(a.data(), b.data(), Size);
    }
   public:
    virtual ~BinaryType() = default;

    virtual const char* data() const{
      return (const char*)data_;
    }

    virtual size_t size() const{
      return Size;
    }

    std::string ToString() const{
      return std::string(data(), size());
    }

    static inline int64_t
    GetSize(){
      return Size;
    }
  };
}

#endif//TOKEN_BINARY_TYPE_H