#ifndef TOKEN_ADDRESS_H
#define TOKEN_ADDRESS_H

#include <set>
#include <uv.h>
#include <glog/logging.h>
#include <leveldb/slice.h>

#include "bitfield.h"

namespace token{
 typedef uint64_t Port;

 class Address{
  public:
   typedef uint64_t RawAddress;

   static inline int
   Compare(const Address& lhs, const Address& rhs){
     int result;
     if((result = memcmp(&lhs.raw_, &rhs.raw_, sizeof(RawAddress))) != 0)
       return result;

     if(lhs.port() < rhs.port()){
       return -1;
     } else if(lhs.port() > rhs.port()){
       return +1;
     }
     return 0;
   }

   struct Comparator{
     bool operator()(const Address& lhs, const Address& rhs) const{
       return lhs < rhs;
     }
   };
  private:
   RawAddress raw_;
   Port port_;
  public:
   Address():
     raw_(0),
     port_(0){
   }
   explicit Address(const char* address, const Port& port);
   explicit Address(const std::string& address, const Port& port):
     Address(address.data(), port){
   }
   Address(const Address& rhs) = default;
   ~Address() = default;

   RawAddress raw() const{
     return raw_;
   }

   Port port() const{
     return port_;
   }

   const char* address() const;

   std::string ToString() const;
   Address& operator=(const Address& rhs) = default;

   friend bool operator==(const Address& lhs, const Address& rhs){
     return Compare(lhs, rhs) == 0;
   }

   friend bool operator!=(const Address& lhs, const Address& rhs){
     return Compare(lhs, rhs) != 0;
   }

   friend bool operator<(const Address& lhs, const Address& rhs){
     return Compare(lhs, rhs) < 0;
   }

   friend bool operator>(const Address& lhs, const Address& rhs){
     return Compare(lhs, rhs) > 0;
   }

   friend std::ostream& operator<<(std::ostream& stream, const Address& rhs){
     return stream << rhs.ToString();
   }
 };

 static inline bool
 GetAddress(const Address& val, struct sockaddr_in& result){
   VERIFY_UVRESULT2(ERROR, uv_ip4_addr(val.address(), val.port(), &result), "cannot get ipv4 address");
   return true;
 }

 typedef std::set<Address, Address::Comparator> AddressList;
 typedef std::string Hostname;

 static inline std::ostream&
 operator<<(std::ostream& stream, const AddressList& val){
   stream << "[";
   auto iter = val.begin();
   do{
     stream << (*iter++);
     if(iter == val.end())
       return stream << "]";
     stream << ", ";
   } while(true);
 }

 class AddressResolver{
  public:
   AddressResolver() = default;
   ~AddressResolver() = default;

   bool ResolveAddresses(const Hostname& hostname, AddressList& results) const;

   static inline bool
   Resolve(const Hostname& hostname, AddressList& results){
     AddressResolver resolver;
     return resolver.ResolveAddresses(hostname, results);
   }

   static inline Address
   Resolve(const Hostname& hostname){
     AddressList results;
     if(!Resolve(hostname, results)){
       LOG(ERROR) << "cannot resolve address list for hostname: " << hostname;
       return Address();
     }

     DLOG(INFO) << "resolved " << hostname << " := " << results;
     return (*results.begin());
   }
 };
}

#endif //TOKEN_ADDRESS_H