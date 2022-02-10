#ifndef TOKEN_PRODUCT_H
#define TOKEN_PRODUCT_H

#include <string>
#include <cstring>
#include <sstream>
#include <algorithm>
#include <leveldb/slice.h>

namespace token{
 class Product{
   /**
    * Fixed-length string representing a Product Id.
    */
  public:
   static constexpr const uint64_t kMaxLength = 64;
   static constexpr const uint64_t kSize = kMaxLength;

   static inline int
   Compare(const Product& lhs, const Product& rhs){
     return strncmp(lhs.c_str(), rhs.c_str(), kMaxLength);
   }

   static inline int
   Compare(const Product& lhs, const std::string& rhs){
     return strncmp(lhs.c_str(), rhs.c_str(), kMaxLength);
   }
  private:
   uint8_t data_[kSize];

   inline void
   CopyFrom(const uint8_t* data, uint64_t size){
     memcpy(data_, data, std::min(size, kSize));
   }
  public:
   Product():
     data_(){
     clear();
   }
   Product(const uint8_t* data, uint64_t length):
     data_(){
     CopyFrom(data, length);
   }
   Product(const char* data, uint64_t length):
     Product((const uint8_t*)data, length){
   }
   explicit Product(const std::string& val):
     Product(val.data(), val.length()){
   }
   Product(const Product& rhs):
     data_(){
     CopyFrom(rhs.data(), rhs.length());
   }
   ~Product() = default;

   const uint8_t* data() const{
     return data_;
   }

   const char* c_str() const{
     return (const char*)data();
   }

   uint64_t length() const{
     return strlen(c_str());
   }

   void clear() const{
     memset((void*)data_, 0, kSize);
   }

   const uint8_t* begin() const{
     return data_;
   }

   const uint8_t* end() const{
     return data() + kSize;
   }

   bool empty() const{
     return std::all_of(begin(), end(), [](const uint8_t val){
       return val == 0;
     });
   }

   Product& operator=(const std::string& val){
     //TODO: check for redundant assignments
     CopyFrom((const uint8_t*)val.data(), val.length());
     return *this;
   }

   explicit operator std::string() const{
     return { c_str(), length() };
   }

   explicit operator leveldb::Slice() const{
     return { c_str(), length() };
   }

   friend std::ostream& operator<<(std::ostream& stream, const Product& val){
     return stream << "Product(" << std::string((const char*)val.data(), val.length()) << ")";
   }

   friend bool operator==(const Product& lhs, const Product& rhs){
     return Compare(lhs, rhs) == 0;
   }

   friend bool operator==(const Product& lhs, const std::string& rhs){
     return Compare(lhs, rhs) == 0;
   }

   friend bool operator!=(const Product& lhs, const Product& rhs){
     return Compare(lhs, rhs) != 0;
   }

   friend bool operator!=(const Product& lhs, const std::string& rhs){
     return Compare(lhs, rhs) != 0;
   }

   friend bool operator<(const Product& lhs, const Product& rhs){
     return Compare(lhs, rhs) < 0;
   }

   friend bool operator<(const Product& lhs, const std::string& rhs){
     return Compare(lhs, rhs) < 0;
   }

   friend bool operator>(const Product& lhs, const Product& rhs){
     return Compare(lhs, rhs) > 0;
   }

   friend bool operator>(const Product& lhs, const std::string& rhs){
     return Compare(lhs, rhs) < 0;
   }
 };

#ifdef TOKEN_JSON_EXPORT
  namespace json{
    static inline bool
    Write(Writer& writer, const Product& val){
      JSON_STRING(writer, val.ToString());
      return true;
    }

    static inline bool
    SetField(Writer& writer, const char* name, const Product& val){
      JSON_KEY(writer, name);
      return Write(writer, val);
    }
  }
#endif//TOKEN_JSON_EXPORT
}

#endif//TOKEN_PRODUCT_H