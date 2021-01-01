#ifndef TOKEN_BITFIELD_H
#define TOKEN_BITFIELD_H

#include "common.h"

namespace Token{
  template<typename S, typename T, int pos, int size>
  class BitField{
   public:
    static bool IsValid(T value){
      return (static_cast<S>(value) & ~((kUWordOne << size) - 1)) == 0;
    }

    static S Mask(){
      return (kUWordOne << size) - 1;
    }

    static S MaskInPlace(){
      return ((kUWordOne << size) - 1) << pos;
    }

    static int Shift(){
      return pos;
    }

    static int BitSize(){
      return size;
    }

    static S Encode(T value){
      return static_cast<S>(value) << pos;
    }

    static T Decode(S value){
      return static_cast<T>((value >> pos) & ((kUWordOne << size) - 1));
    }

    static S Update(T value, S original){
      return (static_cast<S>(value) << pos) | (~MaskInPlace() & original);
    }
  };
}

#endif //TOKEN_BITFIELD_H