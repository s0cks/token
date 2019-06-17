#include "node/message.h"
#include "blockchain.pb.h"

namespace Token{
    BlockMessage* BlockMessage::DecodeBlockMessage(Token::ByteBuffer* bb){
        return new BlockMessage(Block::Load(bb));
    }

    google::protobuf::Message*
    BlockMessage::GetRaw() const{
        return GetBlock()->GetRaw();
    }

    void Message::Encode(Token::ByteBuffer* bb){
        bb->PutInt(static_cast<uint32_t>(GetType()));
        google::protobuf::Message* raw = GetRaw();
        bb->Resize(raw->ByteSizeLong());
        raw->SerializeToArray((char*)bb->GetBytes(), raw->ByteSizeLong());
    }

    Message* Message::Decode(ByteBuffer* bb){
        Message::Type type = static_cast<Type>(bb->GetInt());
        switch(type){
            case Type::kBlockMessage: return BlockMessage::DecodeBlockMessage(bb);
            default: return nullptr;
        }
    }
}