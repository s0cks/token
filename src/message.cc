#include "message.h"

namespace Token{
#define DEFINE_MESSAGE_TYPECHECK(Name) \
    Name##Message* Name##Message::As##Name(){ return this; }
    FOR_EACH_MESSAGE(DEFINE_MESSAGE_TYPECHECK)
#undef DEFINE_MESSAGE_TYPECHECK

    Message* Message::Decode(uint32_t type, Token::ByteBuffer *buff){
        switch(type){
            case kIllegalMessage: return nullptr;
            case kGetHeadMessage: return GetHeadMessage::Decode(buff);
            default: return nullptr;
        }
    }
}