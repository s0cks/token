#ifndef TOKEN_PEER_H
#define TOKEN_PEER_H

namespace token{
  typedef std::set<NodeAddress> PeerList;

  static inline int64_t
  GetBufferSize(const PeerList& peers){
    int64_t size = 0;
    size += sizeof(int64_t);
    size += (peers.size() * NodeAddress::kSize);
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

  static inline bool
  Encode(const BufferPtr& buffer, const PeerList& peers){
    if(!buffer->PutLong(peers.size()))
      return false;
    for(auto& it : peers){
      if(!it.Write(buffer))
        return false;
    }
    return true;
  }

  static inline bool
  Decode(const BufferPtr& buffer, PeerList& peers){
    int64_t len = buffer->GetLong();
    for(int64_t idx = 0; idx < len; idx++)
      if(!peers.insert(NodeAddress(buffer)).second)
        return false;
    return true;
  }
}

#endif//TOKEN_PEER_H