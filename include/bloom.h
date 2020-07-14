#ifndef TOKEN_BLOOM_H
#define TOKEN_BLOOM_H

#include <bitset>
#include "common.h"
#include "uint256_t.h"

namespace Token{
    class BloomFilter{
    private:
        static const size_t kResultSizeBytes = 32;
        static const size_t kStorageSize = 65536;
        static const size_t kBytesPerHashFunction = 4;

        size_t num_hashes_;
        std::bitset<kStorageSize> bits_;
    public:
        BloomFilter(size_t num_hashes=4):
            num_hashes_(num_hashes),
            bits_(){}
        ~BloomFilter(){}

        void Put(const uint256_t& hash){
            uint16_t* hashes = (uint16_t*)hash.data();
            for(size_t idx = 0; idx < num_hashes_; idx++){
                bits_[hashes[idx]] = true;
            }
        }

        bool Contains(const uint256_t& hash) const{
            uint16_t* hashes = (uint16_t*)hash.data();
            for(size_t idx = 0; idx < num_hashes_; idx++){
                if(!bits_[hashes[idx]]) return false;
            }
            return true;
        }
    };
}

#endif //TOKEN_BLOOM_H