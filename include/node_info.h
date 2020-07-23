#ifndef TOKEN_NODE_INFO_H
#define TOKEN_NODE_INFO_H

#include "address.h"

//TODO:
// - merge w/ address.h
// - refactor
// - add PeerInfo class
namespace Token{
    class NodeInfo{
    public:
        typedef Proto::BlockChainServer::NodeInfo RawType;
    private:
        uuid_t node_id_;
        NodeAddress node_addr_;
    public:
        NodeInfo():
            node_id_(),
            node_addr_(){
            uuid_generate_time_safe(node_id_);
        }
        NodeInfo(const uv_tcp_t* handle):
            node_id_(),
            node_addr_(handle){
            uuid_generate_time_safe(node_id_);
        }
        NodeInfo(const NodeAddress& addr):
            node_id_(),
            node_addr_(addr){
            uuid_generate_time_safe(node_id_);
        }
        NodeInfo(const std::string& node_id, const NodeAddress& addr):
            node_id_(),
            node_addr_(addr){
            uuid_parse(node_id.c_str(), node_id_);
        }
        NodeInfo(const std::string& node_id, const std::string& address, uint32_t port):
            node_id_(),
            node_addr_(address, port){
            uuid_parse(node_id.c_str(), node_id_);
        }
        NodeInfo(const RawType& raw):
            node_id_(),
            node_addr_(raw.address()){
            uuid_parse(raw.node_id().c_str(), node_id_);
        }
        NodeInfo(const NodeInfo& other):
            node_id_(),
            node_addr_(other.node_addr_){
            uuid_copy(node_id_, other.node_id_);
        }
        ~NodeInfo(){}

        std::string GetNodeID() const{
            char uuid_str[37];
            uuid_unparse(node_id_, uuid_str);
            return std::string(uuid_str);
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
            uuid_copy(node_id_, other.node_id_);
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

        friend RawType& operator<<(RawType& raw, const NodeInfo& info){
            raw.set_node_id(info.GetNodeID());
            (*raw.mutable_address()) << info.GetNodeAddress();
            return raw;
        }
    };

    class PeerSession;
    class PeerInfo{
    public:
        enum State{ // TODO: remove this / merge this w/ Session::State
            kConnecting,
            kConnected,
            kDisconnected
        };
    private:
        State state_;
        NodeInfo info_;
    public:
        PeerInfo(PeerSession* session);
        ~PeerInfo() = default;

        NodeInfo GetInfo() const{
            return info_;
        }

        std::string GetID() const{
            return info_.GetNodeID();
        }

        NodeAddress GetAddress() const{
            return info_.GetNodeAddress();
        }

        State GetState() const{
            return state_;
        }

        bool IsConnecting() const{
            return GetState() == kConnecting;
        }

        bool IsConnect() const{
            return GetState() == kConnected;
        }

        bool IsDisconnected() const{
            return GetState() == kDisconnected;
        }
    };
}

#endif //TOKEN_NODE_INFO_H