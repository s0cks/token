#include <openssl/evp.h>
#include <openssl/bio.h>
#include <glog/logging.h>

#include "token/hash.h"
#include "token/timestamp.h"

namespace token{
 std::string BigNumber::HexString() const{
   static const char *hexDigits = "0123456789ABCDEF";

   // Create a string and give a hint to its final size (twice the size
   // of the input binary data)
   std::string hexString;
   hexString.reserve(size() * 2);

   // Run through the binary data and convert to a hex string
   std::for_each(
       const_begin(),
       const_end(),
       [&hexString](uint8_t inputByte) {
         hexString.push_back(hexDigits[inputByte >> 4]);
         hexString.push_back(hexDigits[inputByte & 0x0F]);
       });
   return hexString;
 }

 std::string BigNumber::BinaryString() const{
   std::stringstream ss;
   std::bitset<256> bits;
   for(int idx = 0; idx < size(); idx++){
     auto curr = data()[idx];
     int offset = idx * CHAR_BIT;
     for(int bit = 0; bit < CHAR_BIT; bit++){
       bits[offset] = curr & 1;
       offset++;
       curr >>= 1;
     }
   }
   ss << bits;
   return ss.str();
 }

 namespace sha256{
  uint256 Of(const uint8_t* data, size_t length){
    EVP_MD_CTX* ctx;
    if((ctx = EVP_MD_CTX_new()) == nullptr){
      LOG(ERROR) << "failed to initialize sha256 context.";
      return {};
    }

    EVP_MD* sha256;
    if((sha256 = EVP_MD_fetch(nullptr, "SHA256", nullptr)) == nullptr){
      LOG(ERROR) << "failed to fetch sha256 algorithm.";
      return {};
    }

    if(!EVP_DigestInit_ex(ctx, sha256, nullptr)){
      LOG(ERROR) << "failed to initialize sha256 digest.";
      return {};
    }

    if(!EVP_DigestUpdate(ctx, data, length)){
      LOG(ERROR) << "failed to update sha256 digest.";
      return {};
    }

    auto digest = (uint8_t*) OPENSSL_malloc(EVP_MD_get_size(sha256));
    uint32_t len = 0;
    if(!EVP_DigestFinal_ex(ctx, digest, &len)){
      LOG(ERROR) << "failed to calculate sha256 digest.";
      return {};
    }

    uint256 hash(digest, len);

    OPENSSL_free(digest);
    EVP_MD_free(sha256);
    EVP_MD_CTX_free(ctx);
    return hash;
  }

  uint256 Concat(const uint256& lhs, const uint256& rhs){
    uint8_t data[kDigestSize * 2];
    memcpy(&data[0], lhs.data(), kDigestSize);
    memcpy(&data[kDigestSize], rhs.data(), kDigestSize);
    return Of(data, kDigestSize * 2);
  }

  uint256 FromHex(const char* data, size_t length){
    // mapping of ASCII characters to hex values
    static const uint8_t kHexadecimalHashmap[] = {
        0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, // 01234567
        0x08, 0x09, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // 89:;<=>?
        0x00, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f, 0x00, // @ABCDEFG
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // HIJKLMNO
    };

    uint256 hash;
    for(auto pos = 0; ((pos < (hash.size() * 2)) && (pos < length)); pos += 2){
      uint8_t idx0 = ((uint8_t) data[pos + 0] & 0x1F) ^ 0x10;
      uint8_t idx1 = ((uint8_t) data[pos + 1] & 0x1F) ^ 0x10;
      hash[pos / 2] = (uint8_t) (kHexadecimalHashmap[idx0] << 4) | kHexadecimalHashmap[idx1];
    };
    return hash;
  }

  uint256 Nonce(uint64_t length){
    static random_bytes_engine kRandomBytesEngine(Clock::now().time_since_epoch().count());

    std::vector<uint8_t> data(length);
    std::generate(std::begin(data), std::end(data), std::ref(kRandomBytesEngine));
    return Of(data.data(), length);
  }
 }
}

/*
int sha256_file(char *path, char outputBuffer[65])
{
    FILE *file = fopen(path, "rb");
    if(!file) return -534;

    unsigned char hash[SHA256_DIGEST_LENGTH];
    SHA256_CTX sha256;
    SHA256_Init(&sha256);
    const int bufSize = 32768;
    unsigned char *buffer = malloc(bufSize);
    int bytesRead = 0;
    if(!buffer) return ENOMEM;
    while((bytesRead = fread(buffer, 1, bufSize, file)))
    {
        SHA256_Update(&sha256, buffer, bytesRead);
    }
    SHA256_Final(hash, &sha256);

    sha256_hash_string(hash, outputBuffer);
    fclose(file);
    free(buffer);
    return 0;
}
*/