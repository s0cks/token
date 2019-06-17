#ifndef TOKEN_MESSAGE_H
#define TOKEN_MESSAGE_H

#include "bytes.h"
#include "block.h"

namespace Token{
#define FOR_EACH_MESSAGE(V) \
    V(Block)

#define DEFINE_MESSAGE(Name) \
    protected: \
        virtual Type GetType() const{ return Token::Message::Type::k##Name##Message; } \
        virtual google::protobuf::Message* GetRaw() const; \
    public: \
        const char* GetName() const{ return #Name; } \
        static Name##Message* Decode##Name##Message(ByteBuffer* bb);

#define FORWARD_DECLARE_MESSAGE(Name) \
    class Name##Message;
    FOR_EACH_MESSAGE(FORWARD_DECLARE_MESSAGE)
#undef FORWARD_DECLARE_MESSAGE

    class Message{
    public:
        enum class Type{
            kIllegalType = 0,
#define DECLARE_MESSAGE_TYPE(Name) k##Name##Message,
    FOR_EACH_MESSAGE(DECLARE_MESSAGE_TYPE)
#undef DECLARE_MESSAGE_TYPE
        };
    protected:
        virtual Type GetType() const = 0;
        virtual google::protobuf::Message* GetRaw() const = 0;
    public:
        Message(){}
        virtual ~Message(){}

#define DECLARE_MESSAGE_TYPECHECK(Name) \
    bool Is##Name(){ return As##Name() != nullptr; } \
    virtual Name##Message* As##Name(){ return nullptr; }
    FOR_EACH_MESSAGE(DECLARE_MESSAGE_TYPECHECK)
#undef DECLARE_MESSAGE_TYPECHECK

        void Encode(ByteBuffer* bb);
        virtual const char* GetName() const = 0;

        static Message* Decode(ByteBuffer* bb);

        friend std::ostream& operator<<(std::ostream& stream, const Message& msg){
            stream << "Message('" << msg.GetName() << "')";
            return stream;
        }
    };

    class BlockMessage : public Message{
    private:
        Block* block_;
    public:
        BlockMessage(Block* block):
            block_(block){}
        ~BlockMessage(){}

        Block* GetBlock() const{
            return block_;
        }

        DEFINE_MESSAGE(Block);
    };
}

#endif //TOKEN_MESSAGE_H