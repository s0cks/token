#ifndef TOKEN_MESSAGE_H
#define TOKEN_MESSAGE_H

#include "node.grpc.pb.h"
#include "blockchain.pb.h"
#include "service.grpc.pb.h"
#include "bytes.h"

namespace Token{
#define FOR_EACH_TYPE(V) \
    V(Block, Token::Messages::Block) \
    V(GetHead, Token::Node::Messages::EmptyRequest) \
    V(PeerIdent, Token::Node::Messages::PeerIdentity) \
    V(PeerIdentAck, Token::Node::Messages::PeerIdentAck) \
    V(GetBlock, Token::Node::Messages::GetBlockRequest) \

    /**
     * Block := Contains block information
     * GetHead := Request the head from the peer
     * PeerIdent := Send identity to peer
     * PeerIdentAck := Acknowledge that the peer is good
     */

    class Message{
    public:
        enum class Type{
            kUnknownType = 0,
#define DECLARE_TYPE(Name, Type) k##Name##Message,
            FOR_EACH_TYPE(DECLARE_TYPE)
#undef DECLARE_TYPE
        };
    private:
        Type type_;
        google::protobuf::Message* msg_;

        inline void
        SetType(Type type){
            type_ = type;
        }

        inline google::protobuf::Message*
        GetRaw() const{
            return msg_;
        }

#define DECLARE_DECODE_MESSAGE(Name, Type) \
        bool Decode##Name##Message(uint8_t* bytes, size_t size);
        FOR_EACH_TYPE(DECLARE_DECODE_MESSAGE)
#undef DECLARE_DECODE_MESSAGE
    public:
        Message(Type type, google::protobuf::Message* data):
            type_(type),
            msg_(data->New()){
            GetRaw()->CopyFrom(*data);
        }
        Message(Type type=Type::kUnknownType):
            type_(type),
            msg_(nullptr){}
        Message(const Message& other);
        ~Message(){}

        Type GetType() const{
            return type_;
        }

        bool Encode(uint8_t* bytes, size_t size);
        bool Decode(uint8_t* bytes, size_t size);

#define DECLARE_AS(Name, MType) \
        MType* GetAs##Name##Message();
        FOR_EACH_TYPE(DECLARE_AS)
#undef DECLARE_AS

        size_t
        GetMessageSize() const{
            return GetRaw() ?
                   GetRaw()->ByteSizeLong() :
                   0;
        }

        std::string ToString() const;
    };
}

#endif //TOKEN_MESSAGE_H