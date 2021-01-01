#ifndef TOKEN_BLOOM_H
#define TOKEN_BLOOM_H

#include <bitset>
#include "common.h"
#include "hash.h"

namespace Token{
  class BloomFilter{
   private:
    static const size_t kResultSizeBytes = 32;
    static const size_t kStorageSize = 65536;
    static const size_t kBytesPerHashFunction = 4;

    size_t num_hashes_;
    std::bitset<kStorageSize> bits_;
   public:
    BloomFilter(size_t num_hashes = 4):
        num_hashes_(num_hashes),
        bits_(){}
    BloomFilter(const BloomFilter &filter):
        num_hashes_(filter.num_hashes_),
        bits_(filter.bits_){}
    ~BloomFilter(){}

    void Put(const Hash &hash){
      uint16_t *hashes = (uint16_t *) hash.data();
      for(size_t idx = 0; idx < num_hashes_; idx++){
        bits_[hashes[idx]] = true;
      }
    }

    bool Contains(const Hash &hash) const{
      uint16_t *hashes = (uint16_t *) hash.data();
      for(size_t idx = 0; idx < num_hashes_; idx++){
        if(!bits_[hashes[idx]]) return false;
      }
      return true;
    }

    void operator=(const BloomFilter &filter){
      num_hashes_ = filter.num_hashes_;
      bits_ = filter.bits_;
    }
  };
}

#endif //TOKEN_BLOOM_H