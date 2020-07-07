#ifndef TOKEN_ADDRESS_H
#define TOKEN_ADDRESS_H

#include "common.h"

namespace Token{
    class NodeAddress{
    public:
        typedef Proto::BlockChainServer::NodeAddress RawType;
    private:
        uint32_t address_;
        uint32_t port_;
    public:
        NodeAddress();
        NodeAddress(const RawType& raw);
        NodeAddress(const std::string& address);
        NodeAddress(const std::string& address, uint32_t port);
        NodeAddress(const uv_tcp_t* stream);
        NodeAddress(const NodeAddress& other);
        ~NodeAddress() = default;

        uint32_t GetPort() const{
            return port_;
        }

        std::string GetAddress() const;
        std::string ToString() const;
        bool Get(struct sockaddr_in* addr) const;

        NodeAddress& operator=(const NodeAddress& other){
            address_ = other.address_;
            port_ = other.port_;
            return (*this);
        }

        friend bool operator==(const NodeAddress& a, const NodeAddress& b){
            return a.address_ == b.address_ && a.port_ == b.port_;
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

        friend RawType& operator<<(RawType& stream, const NodeAddress& address){
            stream.set_address(address.address_);
            stream.set_port(address.port_);
            return stream;
        }
    };
}

#endif //TOKEN_ADDRESS_H