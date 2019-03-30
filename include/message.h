#ifndef TOKEN_MESSAGE_H
#define TOKEN_MESSAGE_H

#include "bytes.h"
#include "block.h"

namespace Token{
#define FOR_EACH_MESSAGE(V) \
    V(GetHead) \
    V(GetBlock) \
    V(AppendBlock) \
    V(Block) \
    V(Connect) \
    V(SyncRequest) \
    V(SyncResponse)

#define FORWARD_DECLARE(Name) \
    class Name##Message;
    FOR_EACH_MESSAGE(FORWARD_DECLARE)
#undef FORWARD_DECLARE


#define DECLARE_MESSAGE(Name) \
    virtual const char* GetName() const{ return #Name; } \
    virtual Type GetType() const{ return Type::k##Name##Message; } \
    virtual Name##Message* As##Name();

    class Message{
    public:
        enum Type{
            kIllegalMessage = 0,
#define DECLARE_TYPE(Name) k##Name##Message,
            FOR_EACH_MESSAGE(DECLARE_TYPE)
#undef DECLARE_TYPE
        };
    protected:
        virtual Type GetType() const = 0;
        virtual bool GetRaw(ByteBuffer* bb) = 0;
        virtual uint32_t GetSize() const = 0;
    public:
        Message(){}
        virtual ~Message(){}

#define DECLARE_MESSAGE_TYPECHECK(Name) \
        bool Is##Name() { return As##Name() != nullptr; } \
        virtual Name##Message* As##Name() { return nullptr; }
        FOR_EACH_MESSAGE(DECLARE_MESSAGE_TYPECHECK)
#undef DECLARE_MESSAGE_TYPECHECK

        void Encode(ByteBuffer* bb){
            bb->PutInt(static_cast<uint32_t>(GetType()));
            bb->PutInt(GetSize());
            GetRaw(bb);
        }

        virtual const char* GetName() const = 0;

        static Message* Decode(uint32_t type, ByteBuffer* buff);
    };

    class BlockMessage : public Message{
    private:
        Block* block_;
    protected:
        bool GetRaw(ByteBuffer* bb){
            GetBlock()->Encode(bb);
            return true;
        }

        uint32_t GetSize() const{
            ByteBuffer bb;
            GetBlock()->Encode(&bb);
            return bb.WrittenBytes();
        }
    public:
        BlockMessage(Block* block):
            block_(block){}
        ~BlockMessage(){}

        Block* GetBlock() const{
            return block_;
        }

        DECLARE_MESSAGE(Block);

        static BlockMessage* Decode(ByteBuffer* bb){
            uint32_t size = bb->GetInt();
            return new BlockMessage(Block::Decode(bb));
        }
    };

    class AppendBlockMessage : public Message{
    private:
        Block* block_;
    protected:
        bool GetRaw(ByteBuffer* bb){
            GetBlock()->Encode(bb);
            return true;
        }

        uint32_t GetSize() const{
            ByteBuffer bb;
            GetBlock()->Encode(&bb);
            return bb.WrittenBytes();
        }
    public:
        AppendBlockMessage(Block* block):
            block_(block){}
        ~AppendBlockMessage(){}

        Block* GetBlock() const{
            return block_;
        }

        DECLARE_MESSAGE(AppendBlock);

        static AppendBlockMessage* Decode(ByteBuffer* bb){
            uint32_t size = bb->GetInt();
            return new AppendBlockMessage(Block::Decode(bb));
        }
    };

    class GetBlockMessage : public Message{
    private:
        std::string hash_;
    protected:
        bool GetRaw(ByteBuffer* bb);

        uint32_t GetSize() const{
            return sizeof(uint32_t) + hash_.size();
        }
    public:
        GetBlockMessage(const std::string& hash):
            hash_(hash){}
        ~GetBlockMessage(){}

        std::string GetHash() const{
            return hash_;
        }

        DECLARE_MESSAGE(GetBlock);

        static GetBlockMessage* Decode(ByteBuffer* bb);
    };

    class ConnectMessage : public Message{
    private:
        std::string address_;
        uint32_t port_;
    protected:
        bool GetRaw(ByteBuffer* bb);

        uint32_t GetSize() const{
            return sizeof(uint32_t) + address_.size();
        }
    public:
        ConnectMessage(const std::string& addr, uint32_t port):
            address_(addr),
            port_(port){
        }
        ~ConnectMessage(){}

        std::string GetAddress() const{
            return address_;
        }

        uint32_t GetPort() const{
            return port_;
        }

        DECLARE_MESSAGE(Connect);

        static ConnectMessage* Decode(ByteBuffer* bb);
    };

    class GetHeadMessage : public Message{
    protected:
        bool GetRaw(ByteBuffer* bb){
            //Nothing
            return true;
        }

        uint32_t GetSize() const{
            return 0;
        }
    public:
        GetHeadMessage():
            Message(){}
        ~GetHeadMessage(){}

        DECLARE_MESSAGE(GetHead);

        static GetHeadMessage* Decode(ByteBuffer* bb){
            return new GetHeadMessage;
        }
    };

    class SyncRequestMessage : public Message{
    protected:
        bool GetRaw(ByteBuffer* bb){
            return true;
        }

        uint32_t GetSize() const{
            return 0;
        }
    public:
        SyncRequestMessage(): Message(){}
        ~SyncRequestMessage(){}

        DECLARE_MESSAGE(SyncRequest);

        static SyncRequestMessage* Decode(ByteBuffer* bb){
            uint32_t size = bb->GetInt();
            return new SyncRequestMessage;
        }
    };

    class SyncResponseMessage : public Message{
    private:
        Block* block_;
    protected:
        bool GetRaw(ByteBuffer* bb){
            GetBlock()->Encode(bb);
            return true;
        }

        uint32_t GetSize() const{
            ByteBuffer bb;
            GetBlock()->Encode(&bb);
            return bb.WrittenBytes();
        }
    public:
        SyncResponseMessage(Block* block):
            block_(block){}
        ~SyncResponseMessage(){}

        Block* GetBlock() const{
            return block_;
        }

        DECLARE_MESSAGE(SyncResponse);

        static SyncResponseMessage* Decode(ByteBuffer* bb){
            uint32_t size = bb->GetInt();
            return new SyncResponseMessage(Block::Decode(bb));
        }
    };
}

#endif //TOKEN_MESSAGE_H