#include "address.h"
#include "string_utils.h"

namespace token{
 static inline Address::RawAddress
 GetRawAddressFrom(const char* address){
   //TODO: convert to uint16_t
   int a, b, c, d;
   if(sscanf(address, "%d.%d.%d.%d", &a, &b, &c, &d) != 4)
     return 0;
   return (a << 24) | (b << 16) | (c << 8) | (d);
 }

 static const size_t kAddressStringLength = 19;

 static inline void
 GetAddressAsString(const Address::RawAddress& address, char* data){
   uint16_t a = (address >> 24) & 0xFF;
   uint16_t b = (address >> 16) & 0xFF;
   uint16_t c = (address >> 8) & 0xFF;
   uint16_t d = (address) & 0xFF;
   snprintf(data, kAddressStringLength, "%d.%d.%d.%d", a, b, c, d);
 }

 Address::Address(const char* address, const Port& port):
     raw_(GetRawAddressFrom(address)),
     port_(port){}

 const char* Address::address() const{
   char* data = (char*) malloc(sizeof(char) * kAddressStringLength);
   GetAddressAsString(raw(), data);
   return data;//TODO: fix allocation?
 }

 std::string Address::ToString() const{
   char data[kAddressStringLength];
   GetAddressAsString(raw(), data);

   std::stringstream ss;
   ss << std::string(data) << ":" << port();
   return ss.str();
 }

 bool AddressResolver::ResolveAddresses(const Hostname& hostname, AddressList& results) const{
   void* ptr;

   struct addrinfo hints{};
   memset(&hints, 0, sizeof(hints));
   hints.ai_family = PF_UNSPEC;
   hints.ai_socktype = SOCK_STREAM;
   hints.ai_flags |= AI_CANONNAME;

   Port port;
   std::string address;
   if(hostname.find(':') != std::string::npos){
     std::vector<std::string> parts;
     utils::SplitString(hostname, parts, ':');

     address = parts[0];
     port = strtol(parts[1].data(), nullptr, 10);
   } else{
     address = hostname;
     port = 0;
   }

   struct addrinfo* res;
   int err;
   if((err = getaddrinfo(address.data(), nullptr, &hints, &res)) != 0){
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
     results.emplace(std::string(addrstr), port);
     res = res->ai_next;
   }
   return true;
 }
}