#ifndef TOKEN_PEER_H
#define TOKEN_PEER_H

#include <set>
#include "address.h"

namespace token{
  typedef std::set<NodeAddress> PeerList;

  static inline int64_t
  GetBufferSize(const PeerList& list){
    int64_t size = 0;
    size += sizeof(int64_t);
    size += (list.size() * NodeAddress::kSize);
    return size;
  }

  static std::ostream& operator<<(std::ostream& stream, const PeerList& peers){
    stream << "[";

    auto begin = peers.begin();
    auto end = peers.end();
    while(begin != end){
      stream << (*begin);

      auto peek = begin;
      if((++peek) != end)
        stream << ", ";
      begin++;
    }
    return stream << "]";
  }
}

#endif//TOKEN_PEER_H