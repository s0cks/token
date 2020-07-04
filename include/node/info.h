#ifndef TOKEN_INFO_H
#define TOKEN_INFO_H

#include "node.pb.h"

namespace Token{
    //TODO: rename file
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

    class NodeInfo{
    private:
        std::string node_id_;
        NodeAddress node_addr_;
    public:
        NodeInfo():
            node_id_(),
            node_addr_(){}
        NodeInfo(const uv_tcp_t* handle):
            node_id_(GenerateNonce()), //TODO: fixme collision-proof this
            node_addr_(handle){}
        NodeInfo(const NodeAddress& addr):
            node_id_(),
            node_addr_(addr){}
        NodeInfo(const std::string& node_id, const NodeAddress& addr):
            node_id_(node_id),
            node_addr_(addr){}
        NodeInfo(const std::string& node_id, const std::string& address, uint32_t port):
            node_id_(node_id),
            node_addr_(address, port){}
        NodeInfo(const NodeInfo& other):
            node_id_(other.node_id_),
            node_addr_(other.node_addr_){}
        ~NodeInfo(){}

        std::string GetNodeID() const{
            return node_id_;
        }

        std::string GetAddress() const{
            return GetNodeAddress().GetAddress();
        }

        uint32_t GetPort() const{
            return GetNodeAddress().GetPort();
        }

        NodeAddress GetNodeAddress() const{
            return node_addr_;
        }

        NodeInfo& operator=(const NodeInfo& other){
            node_id_ = other.node_id_;
            node_addr_ = other.node_addr_;
            return (*this);
        }

        friend bool operator==(const NodeInfo& a, const NodeInfo& b){
            return a.node_id_ == b.node_id_;
        }

        friend bool operator!=(const NodeInfo& a, const NodeInfo& b){
            return !operator==(a, b);
        }

        friend bool operator<(const NodeInfo& a, const NodeInfo& b){
            return a.GetAddress() < b.GetAddress();
        }

        friend std::ostream& operator<<(std::ostream& stream, const NodeInfo& info){
            stream << info.GetNodeID();
            stream << "[" << info.GetNodeAddress() << "]";
            return stream;
        }
    };
}

#endif //TOKEN_INFO_H