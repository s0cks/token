#ifndef TOKEN_NODE_INFO_H
#define TOKEN_NODE_INFO_H

#include "node_address.h"

namespace Token{
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

#endif //TOKEN_NODE_INFO_H