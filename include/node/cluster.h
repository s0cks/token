#ifndef TOKEN_CLUSTER_H
#define TOKEN_CLUSTER_H

#include <uv.h>
#include "common.h"

namespace Token{
    class PeerAddress{
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
        GetAddress(uv_tcp_t* handle){
            struct sockaddr_in name;
            int nlen = sizeof(name);
            if(uv_tcp_getpeername(handle, (struct sockaddr*)&name, &nlen)) return 0;
            char addr[16];
            uv_inet_ntop(AF_INET, &name.sin_addr, addr, sizeof(addr));
            return GetAddress(std::string(addr));
        }

        static uint32_t
        GetPort(uv_tcp_t* handle){
            struct sockaddr_in name;
            int nlen = sizeof(name);
            if(uv_tcp_getpeername(handle, (struct sockaddr*)&name, &nlen)) return 0;
            return ntohs(name.sin_port);
        }
    public:
        PeerAddress(const std::string& address, uint32_t port):
            address_(GetAddress(address)),
            port_(port){}
        PeerAddress(uv_tcp_t* handle):
            address_(GetAddress(handle)),
            port_(GetPort(handle)){}
        PeerAddress(const PeerAddress& other):
            address_(other.address_),
            port_(other.port_){}
        ~PeerAddress(){}

        std::string GetAddress() const{
            return GetAddress(address_);
        }

        uint32_t GetPort() const{
            return port_;
        }

        PeerAddress& operator=(const PeerAddress& other){
            address_ = other.address_;
            port_ = other.port_;
            return (*this);
        }

        friend bool operator==(const PeerAddress& a, const PeerAddress& b){
            return a.address_ == b.address_ && a.port_ == b.port_;
        }

        friend bool operator!=(const PeerAddress& a, const PeerAddress& b){
            return !operator==(a, b);
        }

        friend bool operator<(const PeerAddress& a, const PeerAddress& b){
            return a.address_ < b.address_;
        }
    };

    typedef PeerAddress ClusterMember;

    class ClusterGroup{
    private:
        typedef std::vector<ClusterMember> ClusterMemberList;

        ClusterMember self_;
        ClusterMemberList members_;
    public:
        ClusterGroup(const ClusterMember& self):
            self_(self),
            members_(){
            members_.push_back(self);
        }
        ~ClusterGroup(){}

        ClusterMember GetSelf() const{
            return self_;
        }

        size_t GetPosition() const{
            for(int i = 0; i < members_.size(); i++){
                auto& it = members_[i];
                if(it == GetSelf()) return i;
            }
            return -1;
        }

        size_t GetSize() const{
            return members_.size();
        }

        ClusterMemberList::iterator begin(){
            return members_.begin();
        }

        ClusterMemberList::const_iterator begin() const{
            return members_.begin();
        }

        ClusterMemberList::iterator end(){
            return members_.end();
        }

        ClusterMemberList::const_iterator end() const{
            return members_.end();
        }
    };
}

#endif //TOKEN_CLUSTER_H