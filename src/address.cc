#include "address.h"

namespace token{
  static inline uint32_t
  GetAddressFrom(const std::string& address){
    int a, b, c, d;
    uint32_t addr;

    if(sscanf(address.c_str(), "%d.%d.%d.%d", &a, &b, &c, &d) != 4)
      return 0;

    addr = a << 24;
    addr |= b << 16;
    addr |= c << 8;
    addr |= d;
    return addr;
  }

  static inline std::string
  GetAddressAsString(uint32_t address){
    int a, b, c, d;

    a = (address >> 24) & 0xFF;
    b = (address >> 16) & 0xFF;
    c = (address >> 8) & 0xFF;
    d = (address & 0xFF);

    char buff[16];
    snprintf(buff, 16, "%d.%d.%d.%d", a, b, c, d);
    return std::string(buff);
  }

  static uint32_t
  GetAddressFromHandle(const uv_tcp_t* handle){
    struct sockaddr_in name;
    int nlen = sizeof(name);
    if(uv_tcp_getpeername(handle, (struct sockaddr*) &name, &nlen))
      return 0;
    char addr[16];
    uv_inet_ntop(AF_INET, &name.sin_addr, addr, sizeof(addr));
    return GetAddressFrom(std::string(addr));
  }

  static inline uint32_t
  GetPortFromHandle(const uv_tcp_t* handle){
    struct sockaddr_in name;
    int nlen = sizeof(name);
    if(uv_tcp_getpeername(handle, (struct sockaddr*) &name, &nlen))
      return 0;
    return ntohs(name.sin_port);
  }

  static inline uint32_t
  GetPortFrom(const std::string& port){
    return atoi(port.c_str());
  }

  NodeAddress::NodeAddress(const std::string& address):
    address_(0),
    port_(0){
    if(address.find(':') != std::string::npos){
      // has port
      std::vector<std::string> parts;
      SplitString(address, parts, ':');
      address_ = GetAddressFrom(parts[0]);
      port_ = GetPortFrom(parts[1]);
    } else{
      address_ = GetAddressFrom(address);
      port_ = FLAGS_server_port;
    }
  }

  NodeAddress::NodeAddress(const std::string& address, uint32_t port):
    address_(GetAddressFrom(address)),
    port_(port){}

  NodeAddress::NodeAddress(const uv_tcp_t* handle):
    address_(GetAddressFromHandle(handle)),
    port_(GetPortFromHandle(handle)){}

  NodeAddress::NodeAddress(const NodeAddress& other):
    address_(other.address_),
    port_(other.port_){}

  std::string NodeAddress::GetAddress() const{
    return GetAddressAsString(address_);
  }

  std::string NodeAddress::ToString() const{
    std::stringstream ss;
    ss << GetAddress() << ":" << GetPort();
    return ss.str();
  }

  bool NodeAddress::Get(struct sockaddr_in* addr) const{
    std::string address = GetAddress();
    return uv_ip4_addr(address.c_str(), port_, addr) == 0;
  }
}