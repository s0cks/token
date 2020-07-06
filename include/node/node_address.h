#ifndef TOKEN_NODE_ADDRESS_H
#define TOKEN_NODE_ADDRESS_H

#include "common.h"

namespace Token{
    class NodeAddress{
    public:
        typedef Proto::BlockChainServer::NodeAddress RawType;
    private:
        uint32_t address_;
        uint32_t port_;

        static uint32_t
        GetAddress(const std::string& address){
            int a, b, c, d;
            uint32_t addr = 0;

            if(sscanf(address.c_str(), "%d.%d.%d.%d", &a, &b, &c, &d) != 4) return 0;

            addr = a << 24;
            addr |= b << 16;
            addr |= c << 8;
            addr |= d;
            return addr;
        }

        static std::string
        GetAddress(uint32_t address){
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
        GetAddress(const uv_tcp_t* handle){
            struct sockaddr_in name;
            int nlen = sizeof(name);
            if(uv_tcp_getpeername(handle, (struct sockaddr*)&name, &nlen)) return 0;
            char addr[16];
            uv_inet_ntop(AF_INET, &name.sin_addr, addr, sizeof(addr));
            return GetAddress(std::string(addr));
        }

        static uint32_t
        GetPort(const uv_tcp_t* handle){
            struct sockaddr_in name;
            int nlen = sizeof(name);
            if(uv_tcp_getpeername(handle, (struct sockaddr*)&name, &nlen)) return 0;
            return ntohs(name.sin_port);
        }

        static uint32_t
        GetPort(const std::string& port){
            return atoi(port.c_str());
        }
    public:
        NodeAddress():
                address_(0),
                port_(0){}
        NodeAddress(const std::string& address, uint32_t port):
                address_(GetAddress(address)),
                port_(port){}
        NodeAddress(const std::string& address):
                address_(0),
                port_(0){
            if(address.find(':') != std::string::npos){
                std::vector<std::string> parts;
                SplitString(address, parts, ':');
                address_ = GetAddress(parts[0]);
                port_ = GetPort(parts[1]);
            } else{
                address_ = GetAddress(address);
                port_ = FLAGS_port;
            }
        }
        NodeAddress(const uv_tcp_t* handle):
                address_(GetAddress(handle)),
                port_(GetPort(handle)){}
        NodeAddress(const NodeAddress& other):
                address_(other.address_),
                port_(other.port_){}
        NodeAddress(const RawType& raw):
                address_(raw.address()),
                port_(raw.port()){}
        ~NodeAddress(){}

        std::string GetAddress() const{
            return GetAddress(address_);
        }

        uint32_t GetPort() const{
            return port_;
        }

        std::string ToString() const{
            std::stringstream ss;
            ss << GetAddress() << ":" << GetPort();
            return ss.str();
        }

        int GetSocketAddressIn(struct sockaddr_in* addr) const{
            std::string address = GetAddress(address_);
            return uv_ip4_addr(address.c_str(), port_, addr);
        }

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

#endif //TOKEN_NODE_ADDRESS_H