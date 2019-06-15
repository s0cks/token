#ifndef TOKEN_MESSAGE_H
#define TOKEN_MESSAGE_H

#include "bytes.h"
#include "block.h"
#include "utxo.h"

namespace Token{
#define FOR_EACH_REQUEST_MESSAGE(V) \
    V(GetHeadRequest)

#define FOR_EACH_RESPONSE_MESSAGE(V) \
    V(GetHeadResponse)

#define FOR_EACH_MESSAGE(V) \
    FOR_EACH_REQUEST_MESSAGE(V) \
    FOR_EACH_RESPONSE_MESSAGE(V)

#define FORWARD_DECLARE(Name) \
    class Name;
    FOR_EACH_MESSAGE(FORWARD_DECLARE)
#undef FORWARD_DECLARE

#define DECLARE_MESSAGE(Name) \
    protected: \
        bool GetRaw(ByteBuffer* bb); \
        uint32_t GetSize() const; \
    public: \
        const char* GetName() const{ return #Name; } \
        Name* As##Name(); \
        Type GetType() const{ return Type::k##Name##Type; } \
        static Name* Decode(ByteBuffer* bb);

    class Request;
    class Response;

    class Message{
    public:
        enum class Type{
            kIllegalType = 0,
#define DECLARE_MESSAGE_TYPE(Name) k##Name##Type,
        FOR_EACH_MESSAGE(DECLARE_MESSAGE_TYPE)
#undef DECLARE_MESSAGE_TYPE
        };
    private:
        Type type_;
    protected:
        virtual Type GetType() const = 0;
        virtual bool GetRaw(ByteBuffer* bb) = 0;
        virtual uint32_t GetSize() const = 0;
    public:
        Message(Type type){}
        virtual ~Message(){}

#define DECLARE_MESSAGE_TYPECHECK(Name) \
        bool Is##Name(){ return As##Name() != nullptr; } \
        virtual Name* As##Name(){ return nullptr; }
        FOR_EACH_MESSAGE(DECLARE_MESSAGE_TYPECHECK)
#undef DECLARE_MESSAGE_TYPECHECK

        void Encode(ByteBuffer* bb){
            bb->PutInt(static_cast<uint32_t>(GetType()));
            bb->PutInt(GetSize());
            GetRaw(bb);
        }

        virtual Request* AsRequest();
        virtual Response* AsResponse();
        virtual bool IsRequest() const = 0;
        virtual bool IsResponse() const = 0;
        virtual const char* GetName() const = 0;

        static Message* Decode(uint32_t type, ByteBuffer* buff);
    };

    class Client;
    class Request : public Message{
    private:
        std::string id_;
    protected:
        virtual uint32_t GetSize() const;
        virtual bool GetRaw(ByteBuffer* bb) = 0;
    public:
        Request(Type type, const std::string& id):
            id_(id),
            Message(type){}
        virtual ~Request(){}

        bool IsRequest() const{
            return true;
        }

        bool IsResponse() const{
            return false;
        }

        std::string GetId() const{
            return id_;
        }

        virtual bool Handle(Client* client, Response* response) = 0;
    };

#define DECLARE_REQUEST(Name, ResponseType) \
    DECLARE_MESSAGE(Name); \
    typedef bool (ResponseHandler)(Client* client, ResponseType* response); \
    private: \
        ResponseHandler* handler_; \
    public: \
        ResponseHandler* GetResponseHandler() const{ return handler_; } \
        void SetResponseHandler(ResponseHandler* handler){ handler_ = handler; } \
        bool HasResponseHandler() const{ return GetResponseHandler() != nullptr; } \
        bool Handle(Client* client, Response* response);

    class GetHeadRequest : public Request{
        DECLARE_REQUEST(GetHeadRequest, GetHeadResponse);
    public:
        GetHeadRequest(const std::string& id):
            Request(Type::kGetHeadRequestType, id){}
        ~GetHeadRequest(){}
    };

    class Response : public Message{
    private:
        std::string id_;
    protected:
        uint32_t GetSize() const;
        virtual bool GetRaw(ByteBuffer* bb) = 0;
    public:
        Response(Type type, std::string id):
            id_(id),
            Message(type){
        }
        ~Response(){}

        std::string GetId() const{
            return id_;
        }

        bool IsRequest() const{
            return false;
        }

        bool IsResponse() const{
            return true;
        }
    };

    class GetHeadResponse : public Response{
        DECLARE_MESSAGE(GetHeadResponse);
    private:
        Block* block_;
    public:
        GetHeadResponse(const std::string& id, Block* block):
            block_(block),
            Response(Type::kGetHeadResponseType, id){}
        GetHeadResponse(const GetHeadRequest& request, Block* block):
            block_(block),
            Response(Type::kGetHeadResponseType, request.GetId()){}
        ~GetHeadResponse(){}

        Block* Get() const{
            return block_;
        }
    };
}

#endif //TOKEN_MESSAGE_H