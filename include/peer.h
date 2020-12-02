#ifndef TOKEN_PEER_H
#define TOKEN_PEER_H

#include <set>
#include "uuid.h"
#include "address.h"

namespace Token{
    class Peer{
    public:
        static const intptr_t kSize = UUID::kSize + NodeAddress::kSize;

        struct IDComparator{
            bool operator()(const Peer& a, const Peer& b){
                return a.uuid_ < b.uuid_;
            }
        };

        struct AddressComparator{
            bool operator()(const Peer& a, const Peer& b){
                return a.address_ < b.address_;
            }
        };
    private:
        UUID uuid_;
        NodeAddress address_;
    public:
        Peer(const UUID& uuid, const NodeAddress& address):
            uuid_(uuid),
            address_(address){}
        Peer(Buffer* buff):
            uuid_(buff),
            address_(buff){}
        Peer(const Peer& other):
            uuid_(other.uuid_),
            address_(other.address_){}
        ~Peer() = default;

        UUID GetID() const{
            return uuid_;
        }

        NodeAddress GetAddress() const{
            return address_;
        }

        bool Write(Buffer* buff) const{
            uuid_.Write(buff);
            address_.Write(buff);
            return true;
        }

        void operator=(const Peer& other){
            uuid_ = other.uuid_;
            address_ = other.address_;
        }

        friend bool operator==(const Peer& a, const Peer& b){
            return a.uuid_ == b.uuid_
                   && a.address_ == b.address_;
        }

        friend bool operator!=(const Peer& a, const Peer& b){
            return !operator==(a, b);
        }

        friend bool operator<(const Peer& a, const Peer& b){
            return a.address_ < b.address_;
        }

        friend std::ostream& operator<<(std::ostream& stream, const Peer& peer){
            stream << peer.GetID() << "(" << peer.GetAddress() << ")";
            return stream;
        }
    };

    typedef std::set<Peer, Peer::AddressComparator> PeerList;
}

#endif //TOKEN_PEER_H
