#ifndef TOKEN_ADDRESS_H
#define TOKEN_ADDRESS_H

#include <set>
#include <uv.h>
#include <glog/logging.h>
#include <leveldb/slice.h>

#include "json.h"
#include "bitfield.h"

namespace token{
  namespace utils{
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
        bool operator()(const Address& lhs, const Address& rhs){
          return lhs < rhs;
        }
      };
     private:
      RawAddress raw_;
      Port port_;
     public:
      Address():
        raw_(0),
        port_(0){}
      explicit Address(const char* address, const Port& port);
      explicit Address(const std::string& address, const Port& port):
        Address(address.data(), port){}
      Address(const Address& rhs) = default;
      ~Address() = default;

      RawAddress raw() const{
        return raw_;
      }

      Port port() const{
        return port_;
      }

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
    };

    typedef std::set<Address, Address::Comparator> AddressList;
    typedef std::string Hostname;

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
        return (*results.begin());
      }
    };
  }

  namespace json{
    static inline bool
    Write(Writer& writer, const utils::Address& val){
      JSON_STRING(writer, val.ToString());
      return true;
    }

    static inline bool
    SetField(Writer& writer, const char* name, const utils::Address& val){
      JSON_KEY(writer, name);
      return Write(writer, val);
    }
  }
}

#endif //TOKEN_ADDRESS_H