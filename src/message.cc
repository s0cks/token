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
    Name* Name::As##Name(){ return this; }
    FOR_EACH_MESSAGE(DEFINE_MESSAGE_TYPECHECK)
#undef DEFINE_MESSAGE_TYPECHECK

    Message* Message::Decode(uint32_t type, Token::ByteBuffer *buff){
        std::cout << "Decoding type: " << type << std::endl;
        switch(static_cast<Type>(type)){
#define DECLARE_DECODE(Name) \
    case Type::k##Name##Type: return Name::Decode(buff);
    FOR_EACH_MESSAGE(DECLARE_DECODE);
#undef DECLARE_DECODE
            case Type::kIllegalType:
            default: return nullptr;
        }
    }

    Request* Message::AsRequest(){
        if(!IsRequest()) return nullptr;
        return static_cast<Request*>(this);
    }

    Response* Message::AsResponse(){
        if(!IsResponse()) return nullptr;
        return static_cast<Response*>(this);
    }

    uint32_t Request::GetSize() const{
        return (sizeof(uint32_t) * 2) + GetId().size();
    }

    uint32_t Response::GetSize() const{
        return (sizeof(uint32_t) * 2) + GetId().size();
    }

    uint32_t GetHeadRequest::GetSize() const{
        return GetHeadRequest::Request::GetSize();
    }

    bool Request::GetRaw(Token::ByteBuffer* bb){
        EncodeString(bb, GetId());
        return true;
    }

    bool Response::GetRaw(Token::ByteBuffer* bb){
        EncodeString(bb, GetId());
        return true;
    }

    bool GetHeadRequest::GetRaw(Token::ByteBuffer* bb){
        return Request::GetRaw(bb);
    }

    uint32_t GetHeadResponse::GetSize() const{
        return GetHeadResponse::Response::GetSize();
    }

    bool GetHeadResponse::GetRaw(Token::ByteBuffer* bb){
        EncodeString(bb, GetId());
        Get()->Encode(bb);
        return true;
    }

    GetHeadRequest* GetHeadRequest::Decode(Token::ByteBuffer* bb){
        uint32_t size = bb->GetInt();
        std::string id;
        DecodeString(bb, &id);
        return new GetHeadRequest(id);
    }

    bool GetHeadRequest::Handle(Client* client, Response* response){
        if(!response->IsGetHeadResponse()){
            std::cerr << "Invalid response from request: " << response->GetId() << std::endl;
            return false;
        }
        if(!HasResponseHandler()){
            std::cerr << "No response handler attached for request: " << response->GetId() << std::endl;
            return false;
        }
        return GetResponseHandler()(client, response->AsGetHeadResponse());
    }

    GetHeadResponse* GetHeadResponse::Decode(Token::ByteBuffer* bb){
        uint32_t size = bb->GetInt();
        std::string id;
        DecodeString(bb, &id);
        std::cout << "Decoding block" << std::endl;
        try{
            Block* block = Block::Decode(bb);
            std::cout << "Block: " << (*block) << std::endl;
            return new GetHeadResponse(id, block);
        } catch(const std::exception& err){
            std::cerr << "Error: " << err.what() << std::endl;
        }
        return nullptr;
    }
}