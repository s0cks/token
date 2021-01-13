#ifndef TOKEN_ADDRESS_H
#define TOKEN_ADDRESS_H

#include <set>
#include "utils/buffer.h"

namespace Token{
  class NodeAddress{
   public:
    static const intptr_t kSize = sizeof(uint32_t) + sizeof(uint32_t);

    template<typename Container>
    static inline bool
    ResolveAddresses(const std::string& hostname, Container& results){
      void* ptr;

      struct addrinfo hints;
      memset(&hints, 0, sizeof(hints));
      hints.ai_family = PF_UNSPEC;
      hints.ai_socktype = SOCK_STREAM;
      hints.ai_flags |= AI_CANONNAME;

      std::string address;
      int32_t port = FLAGS_server_port;
      if(hostname.find(":") != std::string::npos){
        std::vector<std::string> parts;
        SplitString(hostname, parts, ':');
        address = parts[0];
        port = atoi(parts[1].data());
      }

      struct addrinfo* res;
      int err;
      if((err = getaddrinfo(address.data(), NULL, &hints, &res)) != 0){
        LOG(WARNING) << "couldn't get " << hostname << "'s ip: " << gai_strerror(err);
        return false;
      }

      char addrstr[100];
      while(res){
        inet_ntop(res->ai_family, res->ai_addr->sa_data, addrstr, 100);
        switch(res->ai_family){
          case AF_INET:{
            ptr = &((struct sockaddr_in*) res->ai_addr)->sin_addr;
            break;
          }
          case AF_INET6:{
            ptr = &((struct sockaddr_in6*) res->ai_addr)->sin6_addr;
            break;
          }
          default:{
            LOG(WARNING) << "unknown ai_family: " << res->ai_family;
            return false;
          }
        }
        inet_ntop(res->ai_family, ptr, addrstr, 100);
        results.insert(NodeAddress(std::string(addrstr), port));
        res = res->ai_next;
      }
      return true;
    }

    static inline NodeAddress
    ResolveAddress(const std::string& hostname){
      std::set<NodeAddress> addresses;
      if(!ResolveAddresses(hostname, addresses)){
        LOG(WARNING) << "couldn't resolve: " << hostname;
        return NodeAddress();
      }
      return (*addresses.begin());
    }
   private:
    uint32_t address_;
    uint32_t port_;
   public:
    NodeAddress(const std::string& address);
    NodeAddress(const std::string& address, uint32_t port);
    NodeAddress(const uv_tcp_t* stream);
    NodeAddress(const NodeAddress& other);
    NodeAddress(const BufferPtr& buff):
      address_(buff->GetUnsignedInt()),
      port_(buff->GetUnsignedInt()){}
    NodeAddress():
      address_(0),
      port_(0){}
    ~NodeAddress() = default;

    uint32_t GetPort() const{
      return port_;
    }

    std::string GetAddress() const;
    std::string ToString() const;
    bool Get(struct sockaddr_in* addr) const;

    bool Write(const BufferPtr& buff) const{
      buff->PutUnsignedInt(address_);
      buff->PutUnsignedInt(port_);
      return true;
    }

    NodeAddress& operator=(const NodeAddress& other){
      address_ = other.address_;
      port_ = other.port_;
      return (*this);
    }

    friend bool operator==(const NodeAddress& a, const NodeAddress& b){
      return a.address_ == b.address_
             && a.port_ == b.port_;
    }

    friend bool operator!=(const NodeAddress& a, const NodeAddress& b){
      return !operator==(a, b);
    }

    friend bool operator<(const NodeAddress& a, const NodeAddress& b){
      return a.address_ < b.address_;
    }

    friend std::ostream& operator<<(std::ostream& stream, const NodeAddress& address){
      stream << address.ToString();
      return stream;
    }
  };
}

#endif //TOKEN_ADDRESS_H