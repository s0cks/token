#ifndef TOKEN_UINT256_H
#define TOKEN_UINT256_H

namespace token{
 using random_bytes_engine = std::independent_bits_engine<std::default_random_engine, CHAR_BIT, uint8_t>;

 class BigNumber{
  protected:
   BigNumber() = default;
  public:
   virtual ~BigNumber() = default;
   virtual const uint8_t* data() const = 0;
   virtual uint64_t size() const = 0;
   virtual void clear() = 0;

   const uint8_t* const_begin() const{
     return data();
   }

   const uint8_t* const_end() const{
     return data() + size();
   }

   std::string HexString() const;
   std::string BinaryString() const;
 };

 template<uint64_t Size>
 class Hash : public BigNumber{
  protected:
   uint8_t data_[Size];

   constexpr Hash():
       BigNumber(),
       data_(){
     memset(data_, 0, sizeof(data_));
   }
   constexpr Hash(const uint8_t* data, size_t length):
       data_(){
     memset(data_, 0, sizeof(data_));
     memcpy(data_, data, std::min(Size, static_cast<uint64_t>(length)));
   }
  public:
   ~Hash() override = default;

   const uint8_t* data() const override{
     return data_;
   }

   uint64_t size() const override{
     return Size;
   }

   void clear() override{
     memset(data_, 0, sizeof(data_));
   }
 };

#define UINT256_SIZE (256 / 8)

 class uint256 : public Hash<UINT256_SIZE>{
  public:
   static constexpr const uint64_t kSize = UINT256_SIZE;

   static inline int
   Compare(const uint256& lhs, const uint256& rhs){
     return memcmp(lhs.data(), rhs.data(), kSize);
   }
  public:
   uint256() = default;
   constexpr uint256(const uint8_t* data, uint64_t size):
       Hash(data, size){
   }
   uint256(const uint256& rhs) = default;
   ~uint256() override = default;

   uint256& operator=(const uint256& rhs) = default;

   uint8_t& operator[](uint64_t index){
     return data_[index];
   }

   uint8_t operator[](uint64_t index) const{
     return data_[index];
   }

   friend std::ostream& operator<<(std::ostream& stream, const uint256& val){
     return stream << val.HexString();
   }

   friend bool operator==(const uint256& lhs, const uint256& rhs){
     return Compare(lhs, rhs) == 0;
   }

   friend bool operator!=(const uint256& lhs, const uint256& rhs){
     return Compare(lhs, rhs) != 0;
   }

   friend bool operator<(const uint256& lhs, const uint256& rhs){
     return Compare(lhs, rhs) < 0;
   }

   friend bool operator>(const uint256& lhs, const uint256& rhs){
     return Compare(lhs, rhs) > 0;
   }
 };

 namespace sha256{
  static const uint64_t kDigestSize = SHA256_DIGEST_LENGTH;
  static const uint64_t kSize = kDigestSize;

  static const uint64_t kDefaultNonceSize = 1024;

  uint256 Of(const uint8_t* data, size_t length);
  uint256 Nonce(uint64_t size = kDefaultNonceSize);
  uint256 Concat(const uint256& lhs, const uint256& rhs);
  uint256 FromHex(const char* data, size_t length);

  static inline uint256
  FromHex(const std::string& data){
    return FromHex(data.data(), data.length());
  }
 }
}

#endif//TOKEN_UINT256_H