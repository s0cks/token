#ifdef TOKEN_ENABLE_SERVER

#include "configuration.h"
#include "peer/peer_session_manager.h"

#include "server/server.h"
#include "server/session.h"
#include "atomic/relaxed_atomic.h"

namespace Token{
/*//    int64_t total_bytes = static_cast<int64_t>(nread);
//    MessageBufferReader reader(buff, total_bytes);
//    while(reader.HasNext()){
//      MessagePtr next = reader.Next();
//      LOG(INFO) << "next: " << next->ToString();
//      switch(next->GetMessageType()){
//#define DEFINE_HANDLER_CASE(Name) \
//        case Message::k##Name##MessageType: \
//            session->Handle##Name##Message(session, std::static_pointer_cast<Name##Message>(next)); \
//            break;
//        FOR_EACH_MESSAGE_TYPE(DEFINE_HANDLER_CASE)
//#undef DEFINE_HANDLER_CASE
//        case Message::kUnknownMessageType:
//        default: //TODO: handle properly
//          break;
//      }
//    }
//    free(buff->base);*/
}

#endif//TOKEN_ENABLE_SERVER