#ifndef TOKEN_MESSAGE_H
#define TOKEN_MESSAGE_H

#include "blockchain.pb.h"
#include "node.pb.h"
#include "service.pb.h"

#include "allocator.h"
#include "bytearray.h"
#include "blockchain.h"

namespace Token{
#define FOR_EACH_TYPE(V) \
    V(Ping, Token::Messages::Node::Nonce) \
    V(Pong, Token::Messages::Node::Nonce) \
    V(Version, Token::Messages::Node::Version) \
    V(GetData, Token::Messages::HashList) \
    V(Inventory, Token::Messages::HashList) \
    V(Block, Token::Messages::Block) \
    V(PeerList, Token::Messages::PeerList)

    /*
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
    public:
        Message(Type type, google::protobuf::Message* data):
            type_(type),
            msg_(data->New()){
            GetRaw()->CopyFrom(*data);
        }
        explicit Message(Type type=Type::kUnknownType):
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
        MType* GetAs##Name();
        FOR_EACH_TYPE(DECLARE_AS)
#undef DECLARE_AS

#define DECLARE_IS(Name, MType) \
        bool Is##Name##Message();
        FOR_EACH_TYPE(DECLARE_IS)
#undef DECLARE_IS

        size_t
        GetMessageSize() const{
            return GetRaw() ?
                   GetRaw()->ByteSizeLong() :
                   0;
        }

        std::string ToString() const;
    };
    */

    class Message : public AllocatorObject{
    public:
        enum class Type{
            kUnknownType = 0,
#define DECLARE_TYPE(Name, Type) k##Name##Message,
            FOR_EACH_TYPE(DECLARE_TYPE)
#undef DECLARE_TYPE
        };
    private:
        Type type_;
        google::protobuf::Message* raw_;

        inline google::protobuf::Message*
        GetRaw(){
            return raw_;
        }

        bool Decode(ByteArray* bytes);
    public:
        Message(ByteArray* bytes);
        Message(Block* block);
        ~Message();

        bool Encode(ByteArray* bytes);
        std::string ToString();

        uint32_t GetMessageSize(){
            return GetRaw()->ByteSizeLong();
        }

#define DECLARE_AS(Name, MType) \
        MType* GetAs##Name();
        FOR_EACH_TYPE(DECLARE_AS)
#undef DECLARE_AS

#define DECLARE_IS(Name, MType) \
        bool Is##Name##Message();
        FOR_EACH_TYPE(DECLARE_IS)
#undef DECLARE_IS
    };
}

#endif //TOKEN_MESSAGE_H