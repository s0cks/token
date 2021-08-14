#ifndef TKN_ATOMIC_BITSET_H
#define TKN_ATOMIC_BITSET_H

#include "common.h"

namespace token{
  namespace atomic{
    class BitVector{
    private:
      uint64_t size_;
      uint64_t asize_;
      uword* data_;
    public:
      static const int kBitsPerByte = 8;
      static const int kBitsPerWord = sizeof(word) * 8;

      explicit BitVector(uint64_t size):
        size_(size),
        asize_(1 + ((size - 1) / kBitsPerWord)){
        data_ = reinterpret_cast<uword*>(malloc(sizeof(uword) * asize_));
        Clear();
      }

      void Clear(){
        for(int i = 0; i < asize_; i++) data_[i] = 0x0;
      }

      void Add(uint64_t i){
        data_[i / kBitsPerWord] |= static_cast<uword>(1 << (i % kBitsPerWord));
      }

      void Remove(uint64_t i){
        data_[i / kBitsPerWord] &= ~static_cast<uword>(1 << (i % kBitsPerWord));
      }

      void Intersect(BitVector* other){
        for(int i = 0; i < asize_; i++) data_[i] = data_[i] & other->data_[i];
      }

      bool Contains(uint64_t i) const{
        uword block = data_[i / kBitsPerWord];
        return (block & static_cast<uword>(1 << (i % kBitsPerWord))) != 0;
      }

      bool AddAll(BitVector* from){
        bool changed = false;
        for(int i = 0; i < asize_; i++){
          uword before = data_[i];
          uword after = data_[i] | from->data_[i];
          if(before != after){
            changed = true;
            data_[i] = after;
          }
        }
        return changed;
      }

      bool KillAndAdd(BitVector* kill, BitVector* gen){
        bool changed = false;
        for(int i = 0; i < asize_; i++){
          uword before = data_[i];
          uword after = data_[i] | (gen->data_[i] & ~kill->data_[i]);
          if(before != after){
            changed = true;
            data_[i] = after;
          }
        }
        return changed;
      }
    };
  }
}

#endif//TKN_ATOMIC_BITSET_H