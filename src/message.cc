#include "message.h"

namespace Token{
    static void EncodeString(ByteBuffer* bb, std::string val){
        uint32_t size = static_cast<uint32_t>(val.size());
        bb->PutInt(size);
        uint8_t* bytes = new uint8_t[size + 1];
        std::strcpy(reinterpret_cast<char*>(bytes), val.c_str());
        bb->PutBytes(bytes, size);
        delete[] bytes;
    }

    static void DecodeString(ByteBuffer* bb, std::string* result){
        uint32_t size = bb->GetInt();
        uint8_t* bytes = new uint8_t[size + 1];
        bb->GetBytes(bytes, size);
        bytes[size] = '\0';
        *result = std::string(reinterpret_cast<char*>(bytes));
        delete[] bytes;
    }

#define DEFINE_MESSAGE_TYPECHECK(Name) \
    Name##Message* Name##Message::As##Name(){ return this; }
    FOR_EACH_MESSAGE(DEFINE_MESSAGE_TYPECHECK)
#undef DEFINE_MESSAGE_TYPECHECK

    Message* Message::Decode(uint32_t type, Token::ByteBuffer *buff){
        switch(type){
            case kIllegalMessage: return nullptr;
#define DECLARE_DECODE(Name) \
    case k##Name##Message: return Name##Message::Decode(buff);
    FOR_EACH_MESSAGE(DECLARE_DECODE)
#undef DECLARE_DECODE
            default: return nullptr;
        }
    }

    bool ConnectMessage::GetRaw(Token::ByteBuffer *bb){
        EncodeString(bb, GetAddress());
        bb->PutInt(GetPort());
        return true;
    }

    ConnectMessage* ConnectMessage::Decode(Token::ByteBuffer *bb){
        uint32_t size = bb->GetInt();
        std::string address;
        DecodeString(bb, &address);
        return new ConnectMessage(address, bb->GetInt());
    }

    bool GetBlockMessage::GetRaw(Token::ByteBuffer* bb){
        EncodeString(bb, GetHash());
        return true;
    }

    GetBlockMessage* GetBlockMessage::Decode(Token::ByteBuffer* bb){
        uint32_t size = bb->GetInt();
        std::string hash;
        DecodeString(bb, &hash);
        std::cout << "Decoded string: " << hash << std::endl;
        return new GetBlockMessage(hash);
    }
}