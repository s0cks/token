#ifndef TOKEN_MESSAGE_H
#define TOKEN_MESSAGE_H

#include "blockchain.pb.h"
#include "node.pb.h"
#include "service.pb.h"

#include "common.h"
#include "block_chain.h"

namespace Token{
#define FOR_EACH_MESSAGE_TYPE(V) \
    V(Ping) \
    V(Pong) \
    V(Transaction) \
    V(Block)

#define FORWARD_DECLARE(Name) class Name##Message;
    FOR_EACH_MESSAGE_TYPE(FORWARD_DECLARE)
#undef FORWARD_DECLARE

    class Message{
    public:
#define DECLARE_TYPE(Name) k##Name##Message,
        enum class Type{
            kUnknownMessage = 0,
            FOR_EACH_MESSAGE_TYPE(DECLARE_TYPE)
        };
#undef DECLARE_TYPE
    public:
        virtual ~Message() = default;
        virtual std::string GetName() const = 0;
        virtual Type GetType() const = 0;
        virtual uint64_t GetSize() const = 0;
        virtual void Encode(uint8_t* bytes, size_t size) = 0;

#define DECLARE_TYPECHECK(Name) \
        virtual Name##Message* As##Name##Message(){ return nullptr; } \
        bool Is##Name##Message(){ return As##Name##Message() != nullptr; }
        FOR_EACH_MESSAGE_TYPE(DECLARE_TYPECHECK);
#undef DECLARE_TYPECHECK

        static Message* Decode(Type type, uint8_t* bytes, uint32_t size);
    };

#define DECLARE_MESSAGE(Name) \
    public: \
        virtual Name##Message* As##Name##Message(); \
        virtual std::string GetName() const{ return #Name; } \
        virtual void Encode(uint8_t* bytes, size_t size); \
        virtual Type GetType() const{ return Type::k##Name##Message; }
#define DECLARE_MESSAGE_WITH_SIZE(Name, Size) \
    DECLARE_MESSAGE(Name); \
    virtual uint64_t GetSize() const{ return Size; }

    class PingMessage : public Message{
    private:
        std::string nonce_;
    public:
        PingMessage(const std::string& nonce):
            nonce_(nonce),
            Message(){

        }
        PingMessage(): PingMessage(GenerateNonce()){}
        ~PingMessage(){}

        std::string GetNonce() const{
            return nonce_;
        }

        friend bool operator==(const PingMessage& a, const PingMessage& b){
            return a.GetNonce() == b.GetNonce();
        }

        friend bool operator!=(const PingMessage& a, const PingMessage& b){
            return !operator==(a, b);
        }

        DECLARE_MESSAGE_WITH_SIZE(Ping, 64);
    };

    class PongMessage : public Message{
    private:
        std::string nonce_;
    public:
        PongMessage(const std::string& nonce):
            nonce_(nonce),
            Message(){

        }
        PongMessage(const PingMessage& ping): PongMessage(ping.GetNonce()){}
        ~PongMessage(){}

        std::string GetNonce() const{
            return nonce_;
        }

        friend bool operator==(const PongMessage& a, const PongMessage& b){
            return a.GetNonce() == b.GetNonce();
        }

        friend bool operator==(const PongMessage& a, const PingMessage& b){
            return a.GetNonce() == b.GetNonce();
        }

        friend bool operator!=(const PongMessage& a, const PongMessage& b){
            return !operator==(a, b);
        }

        friend bool operator!=(const PongMessage& a, const PingMessage& b){
            return !operator==(a, b);
        }

        DECLARE_MESSAGE_WITH_SIZE(Pong, 64);
    };

    class TransactionMessage : public Message{
    private:
        Messages::Transaction raw_;

        Messages::Transaction GetRaw() const{
            return raw_;
        }
    public:
        TransactionMessage(Transaction* tx):
            raw_(),
            Message(){
            raw_ << tx;
        }
        TransactionMessage(const Messages::Transaction& raw):
            raw_(raw),
            Message(){}
        ~TransactionMessage(){}

        Messages::Transaction* GetTransaction(){
            return &raw_;
        }

        uint64_t GetSize() const{
            return GetRaw().ByteSizeLong();
        }

        DECLARE_MESSAGE(Transaction);
    };

    class BlockMessage : public Message{
    private:
        Messages::Block raw_;

        Messages::Block GetRaw() const{
            return raw_;
        }
    public:
        BlockMessage(Block* block):
            raw_(),
            Message(){
            raw_ << block;
        }
        BlockMessage(const Messages::Block& raw):
            raw_(raw),
            Message(){}
        ~BlockMessage(){}

        uint64_t GetSize() const{
            return GetRaw().ByteSizeLong();
        }

        DECLARE_MESSAGE(Block);
    };
}

#endif //TOKEN_MESSAGE_H